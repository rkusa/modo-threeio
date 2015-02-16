#ifndef __threeio__threesaver__
#define __threeio__threesaver__

#include <lx_mesh.hpp>

#include "scene_helper.h"
#include "json_format.h"
#include "threelogmessage.h"

const std::string THREE_FILE_EXTENSION    = "json";

const std::string THREE_IO_GENERATOR_NAME = "ModoExporter";
const std::string THREE_IO_INTERNAL_NAME  = "THREE_43";
const std::string THREE_IO_USER_NAME      = "THREE JSON format 4.3";

class NgonsException : public std::exception {
};

class TrianglesOnlyException : public std::exception {
};

class THREESceneSaver : public CLxSceneSaver, public SceneHelper, public JSONFormat
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
    
    constexpr static const char* const kUserValueThreeGeometryType = "threeio.geometry.type";
    
    enum PolyPass
    {
        kPolypassGeometry,
        kPolypassBufferGeometry,
        kPolypassNormal
    };
    
    PolyPass poly_pass_;
    
    enum GeometryType
    {
        kGeometry = 0,
        kBufferGeometry = 1
    };
    
    GeometryType opt_geometry_type_;
    
    CLxUser_SceneGraph scene_graph_;
    CLxUser_ItemGraph  item_graph_;
    
    UniqueOrderedSet<Vector> positions_;
    UniqueOrderedSet<Vector> normals_;
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