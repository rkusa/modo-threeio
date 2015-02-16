#include "saver.h"

#include <lxlog.h>
#include <lxidef.h>
#include <lxu_queries.hpp>
#include <lxw_locator.hpp>

THREESceneSaver::THREESceneSaver()
{
}

void THREESceneSaver::WriteScene()
{
    CLxUser_Scene scene = SceneObject();
    
    scene.GetChannels(chan_xform_, 0.0);
    
    scene.GetGraph(LXsGRAPH_XFRMCORE, scene_graph_);
    item_graph_.set(scene_graph_);

    CLxLoc_Item scene_item;
    scene.GetItem(ItemType(LXsITYPE_SCENE), scene_item);
    
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
        
        CLxLoc_Item parent;
        if (item.Parent(parent)) {
            continue; // skip non roots
        }
        
        StartObject();
        WriteObject();
        EndObject();
    }
    
    EndArray(); // children
    EndObject();
}

bool THREESceneSaver::WriteObject()
{
    CLxUser_Item item;
    GetItem(item);
    
    Property("uuid", ItemIdentity());
    const char* name = ItemName();
    if (name) {
        Property("name", name);
    }
    
    if (ItemIsA(LXsITYPE_SCENE)) {
        Property("type", "Scene");
    } else if (ItemIsA(LXsITYPE_MESH)) {
        Property("type", "Mesh");
        Property("geometry", ItemIdentity());
    } else if (ItemIsA(LXsITYPE_MESHINST)) {
        Property("type", "Mesh");
        
        CLxUser_SceneService	 service;
        ILxUnknownID source;
        
        if (LXx_OK(service.GetMeshInstSourceItem((ILxUnknownID)item, (void**)&source))) {
            SetItem((ILxUnknownID)source);
            Property("geometry", ItemIdentity());
            SetItem(item);
        }
    } else if (ItemIsA(LXsITYPE_GROUPLOCATOR)) {
        Property("type", "Group");
    } else if (ItemIsA(LXsITYPE_LOCATOR)) {
        // don't set type for locator/null mesh
    } else {
        // skip unsupported types
        return false;
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
    
    if (!ItemVisible()) {
        Property("visible", false);
    }
    
    unsigned child_count;
    item.SubCount(&child_count);
    
    if (child_count == 0) {
        return true; // we're done
    }
    
    StartArray("children");
    
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
    
    return true;
}

void THREESceneSaver::WriteGeometries()
{
    StartScan();
    while (NextMesh()) {
        if (!ItemVisibleForSave() || PointCount() == 0) {
            continue;
        }
        
        StartObject();
        Property("uuid", ItemIdentity());
        Property("type", "Geometry");
        const char* name = ItemName();
        if (name) {
            Property("name", name);
        }
        
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
        
        EndObject(); // data
        EndObject(); // geometry
        
        positions_.clear();
        normals_.clear();
    }
}

void THREESceneSaver::WriteBufferGeometries()
{
    StartScan();
    while (NextMesh()) {
        if (!ItemVisibleForSave() || PointCount() == 0) {
            continue;
        }
        
        StartObject(); // geometry
        Property("uuid", ItemIdentity());
        Property("type", "BufferGeometry");
        const char* name = ItemName();
        if (name) {
            Property("name", name);
        }
        
        StartObject("data");
        StartObject("attributes");
        
        // index
        StartObject("index");
        Property("itemSize", 3);
        Property("type", "Uint16Array");
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
        
        EndObject(); // attributes
        EndObject(); // data
        EndObject(); // geometry
        
        vertices_.clear();
    }
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
        StartObject();
        
        // metadata
        StartObject("metadata");
        Property("version", "4.3");
        Property("type", "Object");
        Property("generator", THREE_IO_GENERATOR_NAME);
        EndObject();
        
        // geometries
        StartArray("geometries");
        
        if (opt_geometry_type_ == kGeometry) {
            WriteGeometries();
        } else {
            WriteBufferGeometries();
        }
        
        EndArray(); // geometries
        
        WriteScene();
        
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
            
            if (opt_save_normals_) {
                // face normal
                double face_normal[3];
                if (PolyNormal(face_normal) && ReallySaving()) {
                    mask += kFaceNormal;
                    
                    indices.push_back(normals_.insert(face_normal));
                }
                
                // vertex normals
                bool hasVertexNormals = false;
                for (unsigned i = 0; i < num_vert; i++) {
                    double vertex_normal[3];
                    
                    if (PolyNormal(vertex_normal, PolyVertex(i))) {
                        if (i == 0) {
                            hasVertexNormals = true;
                            mask += kFaceVertexNormal;
                        }
                    } else {
                        // arbitrary unit normal
                        vertex_normal[0] = 1.0;
                        vertex_normal[1] = vertex_normal[2] = 0.0;
                    }
                    
                    if (!ReallySaving()) {
                        continue;
                    }
                
                    if (hasVertexNormals) {
                        indices.push_back(normals_.insert(vertex_normal));
                    }
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
                
                if (!ReallySaving()) {
                    continue;
                }
                
                auto vertex_index = vertices_.insert(Vertex(position, normal));
                Write(vertex_index);
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

