#include "saver.h"

#include <cctype>
#include <libgen.h>

#include <lxlog.h>
#include <lxidef.h>
#include <lxu_queries.hpp>
#include <lxw_locator.hpp>

THREESceneSaver::THREESceneSaver()
{
}

void THREESceneSaver::WriteScene()
{
    scene_.GetChannels(chan_, LXs_ACTIONLAYER_EDIT);
    scene_.GetChannels(chan_xform_, 0.0);

    scene_.GetGraph(LXsGRAPH_XFRMCORE, scene_graph_);
    item_graph_.set(scene_graph_);

    CLxLoc_Item scene_item;
    scene_.GetItem(ItemType(LXsITYPE_SCENE), scene_item);

    SetItem(scene_item);

    StartObject("object");
    WriteObject();

    StartArray("children");

    StartScan();
    while(NextItem())
    {
        if (!ItemVisibleForSave() || !ItemSupported()) {
            continue;
        }

        CLxLoc_Item item;
        GetItem(item);
        
        // skip non roots, children are writen recursively
        CLxLoc_Item parent;
        if (item.Parent(parent)) {
            continue;
        }

        StartObject();
        WriteObject();
        EndObject();
    }

    EndArray(); // children
    EndObject();
}

void THREESceneSaver::WriteMaterials()
{
    // find all used materials
    StartScan();
    while (NextItem()) {
        if (!ItemVisibleForSave() || (!ItemIsA(LXsITYPE_MESH) && !ItemIsA(LXsITYPE_MESHINST))) {
            continue;
        }
        
        // find used polygon tags
        poly_pass_ = kPolypassMaterial;
        WritePolys(0, false);
        
        std::string item_id = ItemIdentity();
        std::string item_name = ItemName();
        
        std::string source_name;
        if (ItemIsA(LXsITYPE_MESHINST)) {
            CLxUser_Item item, source;
            GetItem(item);
            scene_service_.GetMeshInstSourceItem((ILxUnknownID)item, source);
            source.GetUniqueName(source_name);
        }
        
        // find all item masks
        for (auto it = poly_tags_.begin(); it != poly_tags_.end(); it++) {
            ShaderMask mask;
            
            //  try item mask + poly tag mask
            if (ScanShaderTree(it->c_str(), item_name.c_str(), source_name.c_str())) {
                ShaderLayer layer;
                while (GetNextLayer(layer)) {
                    SetItem(layer.second);
                    
                    // skip disabled materials
                    if (!ChanInt(LXsICHAN_TEXTURELAYER_ENABLE)) {
                        continue;
                    }
                    
                    if (ItemIsA(LXsITYPE_IMAGEMAP)) {
                        images_.insert(ItemIdentity());
                    }
                    
                    // traversing stack from bottom to top,
                    // take the top most material
                    if (ItemIsA(LXsITYPE_ADVANCEDMATERIAL)) {
                        mask = layer.first;
                    }
                }
            }
            
            materials_.insert(mask);
            material_map_[item_id].insert(mask);
        }
        
        poly_tag_ = "";
        poly_tags_.clear();
    }
    
    WriteTextures();
    
    StartArray("materials");
    
    for (auto it = materials_.begin(); it != materials_.end(); it++) {
        WriteMaterial(*it);
    }
    
    EndArray(); // materials
    
    materials_.clear();
    images_.clear();
}

void THREESceneSaver::WriteTextures()
{
    StartArray("images");
    
    for (auto it = images_.begin(); it != images_.end(); it++) {
        CLxUser_Item map;
        if (!scene_.GetItemByIdent(it->c_str(), map)) {
            continue;
        }
        
        SetItem(map);
        if (TxtrImage()) {
            StartObject(); // image
            Property("uuid", ItemIdentity());
            char* path = strdup(ChanString(LXsICHAN_VIDEOSTILL_FILENAME));
            
            if (opt_embed_images_) {
                std::string type(strdup(ChanString(LXsICHAN_VIDEOSTILL_FORMAT)));
                for(unsigned short i = 0; i < type.size(); i++) {
                    type[i] = std::tolower(type[i]);
                }
                if (type == "jpg") {
                    type = "jpeg";
                }
                
                WriteKey("url");
                std::ifstream is(path);
                Write(is, "image/" + type);
            } else {
                // TODO: MakeFileRelative (is buggy)
                Property("url", basename(path));
            }
            
            EndObject(); // image
        }
    }
    
    EndArray(); // images
    
    StartArray("textures");
    
    for (auto it = images_.begin(); it != images_.end(); it++) {
        CLxUser_Item map;
        if (!scene_.GetItemByIdent(it->c_str(), map)) {
            continue;
        }
        
        SetItem(map);
        if (TxtrImage()) {
            StartObject(); // image
            Property("uuid", ItemIdentity());
            Property("image", ItemIdentity());
            EndObject(); // image
        }
    }
    
    EndArray(); // textures
}

void THREESceneSaver::WriteMaterial(const ShaderMask mask)
{
    if (!ScanShaderTree(mask.second.c_str(), mask.first.c_str())) {
        return;
    }
    
    // this does not export every layer, but only the last material
    // in the shader stack for this mask
    CLxUser_Item material, diffuse_map, specular_map, emissive_map, bump_map;
    ShaderLayer layer;
    while (GetNextLayer(layer)) {
        SetItem(layer.second);
        
        // skip disabled materials
        if (!ChanInt(LXsICHAN_TEXTURELAYER_ENABLE)) {
            continue;
        }
        
        if (ItemIsA(LXsITYPE_ADVANCEDMATERIAL)) {
            GetItem(material);
        } else if (ItemIsA (LXsITYPE_IMAGEMAP)) {
            auto fx = LayerEffect();
            
            if (strcmp(fx, LXs_FX_DIFFCOLOR) == 0) {
                GetItem(diffuse_map);
            } else if (strcmp(fx, LXs_FX_SPECCOLOR) == 0) {
                GetItem(specular_map);
            } else if (strcmp(fx, LXs_FX_LUMICOLOR) == 0) {
                GetItem(emissive_map);
            } else if (strcmp(fx, LXs_FX_BUMP) == 0) {
                GetItem(bump_map);
            }
        }
    }

    if (!material.test() || !SetItem(material)) {
        return;
    }
    
    StartObject();
    Property("uuid", mask.first + "." + mask.second); // ItemIdentity());
    Property("type", "MeshPhongMaterial");
    Property("name", ItemName());
    
    double amount;
    LXtVector color;
    
    // diffuse
    amount = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_DIFFAMT);
    
    if (amount > 0) {
        ChanColor(LXsICHAN_ADVANCEDMATERIAL_DIFFCOL, color);
        
        color[0] *= amount;
        color[1] *= amount;
        color[2] *= amount;
        
        WriteKey("color");
        WriteColor(color);
    }
    
    // specular
    amount = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_SPECAMT);
    
    if (amount > 0) {
        ChanColor(LXsICHAN_ADVANCEDMATERIAL_SPECCOL, color);
        
        color[0] *= amount;
        color[1] *= amount;
        color[2] *= amount;
        
        WriteKey("specular");
        WriteColor(color);
    }
    
    // shininess
    amount = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_ROUGH);
    Property("shininess", int((1.0 - amount) * 100));
    
    // emission
    amount = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_RADIANCE);
    
    if (amount > 0) {
        ChanColor(LXsICHAN_ADVANCEDMATERIAL_LUMICOL, color);
        
        color[0] *= amount;
        color[1] *= amount;
        color[2] *= amount;
        
        WriteKey("emissive");
        WriteColor(color);
    }
    
    // side
    int double_sided = ChanInt(LXsICHAN_ADVANCEDMATERIAL_DBLSIDED);
    if (double_sided == 1) {
        Property("side", 2);
    }
    
//    // blend mode
//    int blending = ChanInt(LXsICHAN_TEXTURELAYER_BLEND);
//    switch (blending) {
//        case 0: // normal
//            Property("blending", 1);
//            break;
//        case 1: // add
//            Property("blending", 2);
//            break;
//        case 2: // substract
//            Property("blending", 3);
//            break;
//        case 6: // multiplay
//            Property("blending", 4);
//            break;
//        default:
//            // TODO: warn for non supported blending
//            break;
//    }
    
    // opacity
    amount = ChanFloat(LXsICHAN_ADVANCEDMATERIAL_TRANAMT);
    if (amount > 0) {
        Property("opacity", 1.0 - amount);
        Property("transparent", true);
    }
//    else {
//        // opacity is used for the layer
//        amount = ChanFloat(LXsICHAN_TEXTURELAYER_OPACITY);
//        Property("opacity", amount);
//    }
    
    // diffuse map
    if (diffuse_map.test() && SetItem(diffuse_map) && TxtrImage()) {
        Property("map", ItemIdentity());
    }
    
    // specular map
    if (specular_map.test() && SetItem(specular_map) && TxtrImage()) {
        Property("specularMap", ItemIdentity());
    }
    
    // ambiente map
    if (emissive_map.test() && SetItem(emissive_map) && TxtrImage()) {
        Property("envMap", ItemIdentity());
    }
    
    // bump map
    if (bump_map.test() && SetItem(bump_map) && TxtrImage()) {
        Property("bumpMap", ItemIdentity());
    }
    
    EndObject(); // material
    
    // TODO:
    // types: BasicMaterial, ShaderMaterial, MeshFaceMaterial
    // props: reflectivity
}

void THREESceneSaver::WriteGeometries()
{
    StartScan();
    while (NextMesh()) {
        if (!ItemVisibleForSave() || PointCount() == 0) {
            continue;
        }
        
        // get all used materials masks
        poly_pass_ = kPolypassMaterial;
        WritePolys(0, false);
        
        // select uv map
        CLxUser_Mesh user_mesh;
        CLxUser_MeshMap mesh_map;
        
        if (opt_save_uvs_ && ChanObject(LXsICHAN_MESH_MESH, user_mesh)) {
            user_mesh.GetMaps(mesh_map);
            
            mesh_map.FilterByType(LXi_VMAP_TEXTUREUV);
            MeshMapVisitor visitor(&mesh_map);
            mesh_map.Enum(&visitor);
            
            if (visitor.names().size() > 0) {
                // select first uv
                // TODO: export all uvs?
                has_uvs_ = SetMap(LXi_VMAP_TEXTUREUV, visitor.names().begin()->c_str());
            }
        }
        
        // create a geometry for each material tag
        for (auto it = poly_tags_.begin(); it != poly_tags_.end(); it++) {
            poly_tag_ = *it;
            
            if (opt_geometry_type_ == kGeometry) {
                WriteGeometry();
            } else {
                WriteBufferGeometry();
            }
            
            vertices_.clear();
            positions_.clear();
            normals_.clear();
            uvs_.clear();
        }
        
        poly_tags_.clear();
        poly_tag_ = "";
        has_uvs_ = false;
    }
}

void THREESceneSaver::WriteGeometry()
{
    StartObject();
    Property("uuid", ItemIdentity() + poly_tag_);
    Property("type", "Geometry");
    
    StartObject("data");
    
    // faces
    StartArray("faces");
    poly_pass_ = kPolypassGeometry;
    WritePolys(0, true); // Enable unified polygon material mapping.
    EndArray();
    
    // vertices
    StartArray("vertices");
    for (auto position : positions_) {
        Write(position.x);
        Write(position.y);
        Write(position.z);
    }
    EndArray();
    
    // normals
    if (opt_save_normals_) {
        StartArray("normals");
        for (auto normal : normals_) {
            Write(normal.x);
            Write(normal.y);
            Write(normal.z);
        }
        EndArray(); // normals
    }
    
    if (opt_save_uvs_ && has_uvs_) {
        StartArray("uvs");
        StartArray(); // uv layer 0
        for (auto uv : uvs_) {
            Write(uv.x);
            Write(uv.y);
        }
        EndArray(); // uv layer 0
        EndArray(); // uvs
    }
    
    EndObject(); // data
    EndObject(); // geometry
}

void THREESceneSaver::WriteBufferGeometry()
{
    StartObject(); // geometry
    Property("uuid", ItemIdentity() + poly_tag_);
    Property("type", "BufferGeometry");
    
    StartObject("data");
    StartObject("attributes");
    
    // index
    StartObject("index");
    Property("itemSize", 1);
    Property("type", "Uint32Array");
    StartArray("array");
    // traverse faces
    poly_pass_ = kPolypassBufferGeometry;
    WritePolys(0, true); // Enable unified polygon material mapping.
    EndArray(); // array
    EndObject(); // index
    
    // positions
    StartObject("position");
    Property("itemSize", 3);
    Property("type", "Float32Array");
    StartArray("array");
    for (auto vertex : vertices_) {
        auto position = vertex.position();
        Write(position.x);
        Write(position.y);
        Write(position.z);
    }
    EndArray(); // array
    EndObject(); // position
    
    // normals
    if (opt_save_normals_) {
        StartObject("normal");
        Property("itemSize", 3);
        Property("type", "Float32Array");
        StartArray("array");
        for (auto vertex : vertices_) {
            auto normal = vertex.normal();
            Write(normal.x);
            Write(normal.y);
            Write(normal.z);
        }
        EndArray(); // array
        EndObject(); // normal
    }
    
    // uvs
    if (opt_save_uvs_ && has_uvs_) {
        StartObject("uv");
        Property("itemSize", 2);
        Property("type", "Float32Array");
        StartArray("array");
        for (auto vertex : vertices_) {
            auto uv = vertex.uv();
            Write(uv.x);
            Write(uv.y);
        }
        EndArray(); // array
        EndObject(); // uv
    }
    
    EndObject(); // attributes
    EndObject(); // data
    EndObject(); // geometry
}

void THREESceneSaver::WriteObject()
{
    CLxUser_Item item;
    GetItem(item);
    
    Property("uuid", ItemIdentity());
    const char* name = ItemName();
    if (name) {
        Property("name", name);
    }
    
    LXtMatrix4 transform = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 },
    };
    
    CLxLoc_Locator locator;
    if (locator.set(item)) {
        locator.LocalTransform4(chan_xform_, transform);
    }
    
    StartArray("matrix");
    for (unsigned col = 0; col < 4; ++col) {
        for (unsigned row = 0; row < 4; ++row) {
            Write(transform[col][row]);
        }
    }
    EndArray();
    
    // poly tag -> item tag
    std::map<std::string, std::string> materials;
    auto iter = material_map_.find(ItemIdentity());
    if (iter != material_map_.end()) {
        for (auto it = iter->second.begin(); it != iter->second.end(); it++) {
            materials[it->second] = it->first;
        }
    }
    
    if (ItemIsA(LXsITYPE_SCENE)) {
        Property("type", "Scene");
    } else if (ItemIsA(LXsITYPE_MESH) || ItemIsA(LXsITYPE_MESHINST)) {
        if (ItemIsA(LXsITYPE_MESHINST)) {
            CLxUser_Item source;
            scene_service_.GetMeshInstSourceItem((ILxUnknownID)item, source);
            SetItem(source);
        }
        
        poly_pass_ = kPolypassMaterial;
        WritePolys(0, false);
        
        if (poly_tags_.size() == 1) {
            Property("type", "Mesh");
            Property("geometry", ItemIdentity() + *poly_tags_.begin());
            Property("material", materials.begin()->second + '.' + materials.begin()->first);
        }
    } else if (ItemIsA(LXsITYPE_GROUPLOCATOR)) {
        Property("type", "Group");
    } else if (ItemIsA(LXsITYPE_LOCATOR)) {
        // don't set type for locator/null mesh
    }
    
    // TODO:
    //    PerspectiveCamera
    //    OrthographicCamera
    //    AmbientLight
    //    DirectionalLight
    //    PointLight
    //    SpotLight
    //    HemisphereLight
    //    Line
    //    Sprite
    
    if (!ItemVisible()) {
        Property("visible", false);
    }
    
    unsigned child_count;
    item.SubCount(&child_count);
    
    if (child_count > 0 || poly_tags_.size() > 1) {
        StartArray("children");
        
        if (poly_tags_.size() > 1) {
            for (auto it = poly_tags_.begin(); it != poly_tags_.end(); it++) {
                std::string poly_mask, item_mask;
                auto iter = materials.find(*it);
                if (iter != materials.end()) {
                    poly_mask = *it;
                    item_mask = iter->second;
                } else { // not found
                    iter = materials.find("");
                    if (iter != materials.end()) {
                        item_mask = iter->second;
                    }
                }
                
                StartObject();
                Property("uuid", ItemIdentity() + *it);
                Property("type", "Mesh");
                Property("geometry", ItemIdentity() + *it);
                Property("material", item_mask + '.' + poly_mask);
                StartArray("matrix");
                Write(1); Write(0); Write(0); Write(0);
                Write(0); Write(1); Write(0); Write(0);
                Write(0); Write(0); Write(1); Write(0);
                Write(0); Write(0); Write(0); Write(1);
                EndArray(); // matrix
                if (!ItemVisible()) {
                    Property("visible", false);
                }
                EndObject();
            }
        }
        
        for (unsigned i = 0; i < child_count; ++i) {
            CLxUser_Item child;
            item.SubByIndex(i, child);
            
            SetItem(child);
            
            if (!ItemVisibleForSave() || !ItemSupported()) {
                continue;
            }
            
            StartObject();
            WriteObject();
            EndObject();
        }
        
        EndArray(); // children
    }
    
    SetItem(item);
    
    poly_tag_ = "";
    poly_tags_.clear();
    
    return;
}

const bool THREESceneSaver::ItemVisibleForSave() const {
    return opt_save_hidden_ || ItemVisible();
}

const bool THREESceneSaver::ItemSupported() const {
    return ItemIsA(LXsITYPE_MESH) ||
           ItemIsA(LXsITYPE_MESHINST) ||
           ItemIsA(LXsITYPE_GROUPLOCATOR) ||
           ItemIsA(LXsITYPE_LOCATOR);
}

void THREESceneSaver::GetOptions()
{
    CLxReadUserValue	 ruv;

    if (ruv.Query(kUserValueSaveHidden)) {
        opt_save_hidden_ = ruv.GetInt() ? true : false;
    }
    
    if (ruv.Query(kUserValueSaveNormals)) {
        opt_save_normals_ = ruv.GetInt() ? true : false;
    }
    
    if (ruv.Query(kUserValueSaveUVs)) {
        opt_save_uvs_ = ruv.GetInt() ? true : false;
    }
    
    if (ruv.Query(kUserValueEmbedImages)) {
        opt_embed_images_ = ruv.GetInt() ? true : false;
    }

    if (ruv.Query(kUserValueGeometryType)) {
        opt_geometry_type_ = (GeometryType)ruv.GetInt();
    }

    if (ruv.Query(kUserValuePrecisionEnabled)) {
        opt_precision_enabled_ = ruv.GetInt() ? true : false;
    }

    if (ruv.Query(kUserValuePrecisionValue)) {
        opt_precision_value_ = ruv.GetInt();
    }

    if (ruv.Query(kUserValueJSONPretty)) {
        opt_json_pretty_ = ruv.GetInt() ? true : false;
    }
}

/*
 * Layers in the shader tree are selected by mask strings. The initialization
 * method scans the tree recursively and finds all layers that match the masks.
 * The Nextlayer() method steps through that list.
 */
bool THREESceneSaver::ScanShaderTree(const char* poly_mask, const char* item_mask, const char* source_mask)
{
    CLxUser_Item	 render;
    
    layer_.clear();
    current_layer_ = 0;
    
    if (!scene_.GetItem(ItemType(LXsITYPE_POLYRENDER), render)) {
        return false;
    }
    
    if (!TraverseLayers(render, ShaderMask(), poly_mask, item_mask, source_mask)) {
        return false;
    }
    
    return true;
}

bool THREESceneSaver::GetNextLayer(ShaderLayer& layer)
{
    if (current_layer_ >= layer_.size()) {
        return false;
    }
    
    layer = layer_[current_layer_++];
    
    return true;
}

bool THREESceneSaver::TraverseLayers(CLxUser_Item& root, ShaderMask current_mask, const char* poly_mask, const char* item_mask, const char* source_mask)
{
    unsigned child_count;
    if (!LXx_OK(root.SubCount(&child_count))) {
        return false;
    }
    
    /*
     * Grab the shader tree graph to look up item masks.
     */
    CLxUser_SceneGraph scene_graph;
    scene_.GetGraph(LXsGRAPH_SHADELOC, scene_graph);
    CLxUser_ItemGraph item_graph;
    item_graph.set(scene_graph);
    
    CLxUser_Item	 child;
    for (unsigned i = 0; i < child_count; i++) {
        if (!root.GetSubItem(i, child)) {
            return false;
        }
        
        if (!child.IsA(ItemType(LXsITYPE_MASK))) {
            layer_.push_back(ShaderLayer(current_mask, child));
            continue;
        }
        
        // skip disabled layers
        unsigned idx = child.ChannelIndex(LXsICHAN_TEXTURELAYER_ENABLE);
        int enabled;
        if (idx == -1 || !LXx_OK(chan_.Integer(child, idx, &enabled)) || !enabled) {
            continue;
        }
        
        idx = child.ChannelIndex(LXsICHAN_MASK_PTAG);
        const char* poly_tag;
        if (!LXx_OK(chan_.String(child, idx, &poly_tag))) {
            return false;
        }
        
        const char* child_name;
        child.UniqueName(&child_name);
        
        ShaderMask mask;
        
        // check if the layer has a matching poly tag
        if (poly_tag && strlen(poly_tag) > 0) {
            if (poly_mask && strcmp(poly_tag, poly_mask) == 0) {
                mask.second = poly_tag;
            } else {
                continue;
            }
        }
        
        // check if the layer has a matching item mask
        unsigned item_count = item_graph.Forward(child);
        CLxUser_Item layer_mask;
        if (item_count && item_graph.Forward(child, 0, layer_mask)) {
            const char* layer_name;
            layer_mask.UniqueName(&layer_name);
            
            if (layer_name && strlen(layer_name) > 0) {
                if (item_mask && strcmp(layer_name, item_mask) == 0) {
                    mask.first = item_mask;
                } else if (source_mask && strcmp(layer_name, source_mask) == 0) {
                    mask.first = source_mask;
                } else {
                    continue;
                }
            }
        }
        
        // traverse children
        if (!TraverseLayers(child, mask, poly_mask, item_mask, source_mask)) {
            return false;
        }
    }
    
    return true;
}

void THREESceneSaver::ss_Verify()
{
    // TODO: not working ...
//    Message("common", 2020);
//    MessageArg(1, "Test warning");
}

LxResult THREESceneSaver::ss_Save()
{
    GetOptions();
    if (opt_precision_enabled_) {
        precision(opt_precision_value_);
    }
    pretty(opt_json_pretty_);

    LxResult result(LXe_OK);
    log.Setup();

    try {
        scene_ = SceneObject();
        
        StartObject();

        // metadata
        StartObject("metadata");
        Property("version", "4.3");
        Property("type", "Object");
        Property("generator", THREE_IO_GENERATOR_NAME);
        EndObject();
        
        // materials
        WriteMaterials();

        // geometries
        StartArray("geometries");
        WriteGeometries();
        EndArray(); // geometries

        WriteScene();
        
        material_map_.clear();

        EndObject(); // root
    } catch (NgonsException& e) {
        log.Error("ngons are not supported");

        // Force a failure result code, in case no one set it.
        if (LXx_OK(result)) {
            result = LXe_FAILED;
        }
    } catch (TrianglesOnlyException& e) {
        log.Error("when exporting as BufferGeometry, only triangles are supported");

        // Force a failure result code, in case no one set it.
        if (LXx_OK(result)) {
            result = LXe_FAILED;
        }
    } catch (...) {
        log.Error("Saver failed with unknown error.");

        // Force a failure result code, in case no one set it.
        if (LXx_OK(result)) {
            result = LXe_FAILED;
        }
    }

    if (LXx_OK(result)) {
        log.Info("Scene saved successfully.");
    }

    return result;
}

// A point visitor.
void THREESceneSaver::ss_Point()
{
}

// A polygon visitor.
void THREESceneSaver::ss_Polygon()
{
    switch (poly_pass_) {
        case kPolypassGeometry:
        {
            // skip polygons that do not match the current
            // material tag filter
            std::string tag(PolyTag(LXi_PTAG_MATR));
            if (tag != poly_tag_) {
                break;
            }
            
            unsigned num_vert = PolyNumVerts();

            if (num_vert != 3 && num_vert != 4) {
                throw NgonsException();
            }

            unsigned mask = kTriangle;

            if (num_vert == 4) {
                mask += kQuad;
            }

            std::vector<unsigned> indices;

            // positions
            for (unsigned i = 0; i < num_vert; i++) {
                PntSet(PolyVertex(i));

                double position[3];
                PntPosition(position);

                if (!ReallySaving()) {
                    continue;
                }

                indices.push_back(positions_.insert(position));
            }
            
            // uvs
            if (opt_save_uvs_ && has_uvs_) {
                mask += kFaceVertexUv;
                
                for (unsigned i = 0; i < num_vert; i++) {
                    float uv[2];
                    if (!PolyMapValue(uv, PolyVertex(i))) {
                        uv[0] = uv[1] = 0.0f;
                    }
                    
                    if (!ReallySaving()) {
                        continue;
                    }
                    
                    indices.push_back(uvs_.insert(uv));
                }
            }

            if (opt_save_normals_) {
                // face normal
                double face_normal[3];
                if (PolyNormal(face_normal) && ReallySaving()) {
                    mask += kFaceNormal;

                    indices.push_back(normals_.insert(face_normal));
                }

                // vertex normals
                for (unsigned i = 0; i < num_vert; i++) {
                    double vertex_normal[3];

                    if (!PolyNormal(vertex_normal, PolyVertex(i))) {
                        if (i == 0) {
                            break;
                        }
                        
                        // arbitrary unit normal
                        vertex_normal[0] = 1.0;
                        vertex_normal[1] = vertex_normal[2] = 0.0;
                    }
                    
                    if (i == 0) {
                        mask += kFaceVertexNormal;
                    }

                    if (!ReallySaving()) {
                        continue;
                    }

                    indices.push_back(normals_.insert(vertex_normal));
                }
            }

            Write(mask);
            for (unsigned index : indices) {
                Write(index);
            }

            break;
        }
        case kPolypassBufferGeometry:
        {
            // skip polygons that do not match the current
            // material tag filter
            std::string tag(PolyTag(LXi_PTAG_MATR));
            if (tag != poly_tag_) {
                break;
            }
            
            unsigned num_vert = PolyNumVerts();

            if (num_vert != 3) {
                throw TrianglesOnlyException();
            }

            // vertices
            for (unsigned i = 0; i < num_vert; i++) {
                auto vertex_id = PolyVertex(i);
                PntSet(vertex_id);

                double position[3];
                PntPosition(position);

                double normal[3] = { 0, 0, 0 };
                if (opt_save_normals_) {
                    // try vertex normal
                    if (!PolyNormal(normal, vertex_id)) {
                        // try face normal
                        if (!PolyNormal(normal)) {
                            // arbitrary unit normal
                            normal[0] = 1.0;
                            normal[1] = normal[2] = 0.0;
                        }
                    }
                }
                
                float uv[2] = { 0, 0 };
                if (opt_save_uvs_ && has_uvs_) {
                    if (!PolyMapValue(uv, vertex_id)) {
                        uv[0] = uv[1] = 0.0f;
                    }
                }
                
                if (!ReallySaving()) {
                    continue;
                }
                
                Vertex vertex(position, normal, uv);
                auto vertex_index = vertices_.insert(vertex);
                Write(vertex_index);
            }

            break;
        }
        case kPolypassMaterial:
        {
            std::string tag(PolyTag(LXi_PTAG_MATR));
            
            if (poly_tags_.size() == 0 || tag != poly_tag_) {
                poly_tags_.insert(tag);
                poly_tag_ = tag;
            }
            
            break;
        }
    }
}

LXtTagInfoDesc THREESceneSaver::descInfo[] = {
    { LXsSAV_OUTCLASS,      LXa_SCENE },
    { LXsSAV_DOSTYPE,       THREE_FILE_EXTENSION.c_str() },
    { LXsSRV_USERNAME,      THREE_IO_USER_NAME.c_str() },
    { LXsSRV_LOGSUBSYSTEM,	"io-status" },
    { 0 }
};

// Exporting Saver
void initialize()
{
    LXx_ADD_SERVER(Saver, THREESceneSaver, THREE_IO_INTERNAL_NAME.c_str());
}

