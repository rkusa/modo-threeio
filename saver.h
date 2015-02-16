#ifndef __threeio__threesaver__
#define __threeio__threesaver__

#include <lx_action.hpp>
#include <lxu_scene.hpp>

#include "jsonformat.h"
#include "logmessage.h"
#include "types.h"

const std::string THREE_FILE_EXTENSION    = "json";

const std::string THREE_IO_GENERATOR_NAME = "ModoExporter";
const std::string THREE_IO_INTERNAL_NAME  = "THREE_43";
const std::string THREE_IO_USER_NAME      = "THREE JSON format 4.3";

class NgonsException : public std::exception {
};

class TrianglesOnlyException : public std::exception {
};

class THREESceneSaver : public CLxSceneSaver, public JSONFormat
{
public:
    
    THREESceneSaver();
    ~THREESceneSaver() {}
    
    //----------------------------------------------------------------
    //	Standard exporter overrides.
    
    virtual CLxFileFormat *ss_Format() {
        return this;
    }
    
    virtual void     ss_Verify();
    virtual LxResult	 ss_Save() override;
    virtual void		 ss_Point() override;
    virtual void		 ss_Polygon() override;
    
    static LXtTagInfoDesc descInfo[];
    
private:
    
    ThreeLogMessage log;
    
    // face type bytemask (https://github.com/mrdoob/three.js/wiki/JSON-Model-format-3)
    static const unsigned short kTriangle = 0;
    static const unsigned short kQuad = 1;
    static const unsigned short kFaceMaterial = 2;
    static const unsigned short kFaceUv = 4;
    static const unsigned short kFaceVertexUv = 8;
    static const unsigned short kFaceNormal = 16;
    static const unsigned short kFaceVertexNormal = 32;
    static const unsigned short kFaceColor = 64;
    static const unsigned short kFaceVertexColor = 128;
    
    constexpr static const char* const kUserValueSaveHidden = "threeio.save.hidden";
    constexpr static const char* const kUserValueSaveNormals = "threeio.save.normals";
    constexpr static const char* const kUserValueGeometryType = "threeio.geometry.type";
    constexpr static const char* const kUserValuePrecisionEnabled = "threeio.precision.enabled";
    constexpr static const char* const kUserValuePrecisionValue = "threeio.precision.value";
    constexpr static const char* const kUserValueJSONPretty = "threeio.json.pretty";
    
    enum PolyPass
    {
        kPolypassBufferGeometry,
        kPolypassGeometry
    };
    
    PolyPass poly_pass_;
    
    enum GeometryType
    {
        kBufferGeometry = 0,
        kGeometry = 1
    };
    
    CLxUser_ChannelRead chan_xform_;
    
    bool opt_save_hidden_ = false;
    bool opt_save_normals_ = false;
    GeometryType opt_geometry_type_ = kGeometry;
    bool opt_precision_enabled_ = false;
    unsigned opt_precision_value_ = 6;
    bool opt_json_pretty_ = true;
    
    CLxUser_SceneGraph scene_graph_;
    CLxUser_ItemGraph  item_graph_;
    
    UniqueOrderedSet<Vector3> positions_;
    UniqueOrderedSet<Vector3> normals_;
    UniqueOrderedSet<Vertex> vertices_;
    
    bool WriteObject();
    void WriteScene();
    void WriteGeometries();
    void WriteBufferGeometries();
    
    const bool ItemVisibleForSave() const;
    const bool ItemSupported() const;
    void GetOptions();
};

#endif // /* defined(__threeio__threesaver__) */