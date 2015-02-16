#ifndef __threeio__threesaver__
#define __threeio__threesaver__

#include <vector>
#include <map>

#include <lx_action.hpp>
#include <lxu_scene.hpp>

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

struct Vector
{
    Vector(double v[3]) {
        vector_[0] = v[0];
        vector_[1] = v[1];
        vector_[2] = v[2];
    }
    
    bool operator==(const Vector& rhs)
    {
        return (vector_[0] == rhs[0] &&
                vector_[1] == rhs[1] &&
                vector_[2] == rhs[2]);
    }
    
    bool operator!=(const Vector& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vector& rhs) const
    {
        for (unsigned i = 0; i < 3; ++i) {
            if (vector_[i] > rhs[i]) {
                return false;
            }
            
            if (vector_[i] < rhs[i]) {
                return true;
            }
        }
        
        // equal
        return false;
    }
    
    const double& operator[](const int i) const
    {
        return vector_[i];
    }
    
private:
    double vector_[3];
};

struct Vertex {
    Vertex(double p[3], double n[3]) : position_(p), normal_(n) {
    }
    
    bool operator==(const Vertex& rhs)
    {
        return position_ == rhs.position() && normal_ == rhs.normal();
    }
    
    bool operator!=(const Vertex& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vertex& rhs) const
    {
        if (position_ < rhs.position()) {
            return true;
        }
        
        if (normal_ < rhs.normal()) {
            return true;
        }
        
        return false;
    }
    
    const Vector position() const {
        return position_;
    }
    
    const Vector normal() const {
        return normal_;
    }
    
private:
    Vector position_;
    Vector normal_;
};

template <class T>
struct UniqueOrderedSet {
public:
    unsigned insert(T vector) {
        auto iter = map_.find(vector);
        if (iter != map_.end()) {
            return iter->second;
        } else {
            map_[vector] = index_;
            order_.push_back(vector);
            return index_++;
        }
    }
    
    void clear() {
        map_.clear();
        order_.clear();
        index_ = 0;
    }
    
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    
    iterator begin() { return order_.begin(); }
    const_iterator begin() const { return order_.begin(); }
    const_iterator cbegin() const { return order_.cbegin(); }
    iterator end() { return order_.end(); }
    const_iterator end() const { return order_.end(); }
    const_iterator cend() const { return order_.cend(); }
    
private:
    
    std::map<T, unsigned> map_;
    std::vector<T> order_; // TODO: disposed?
    unsigned index_ = 0;
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
    
    constexpr static const char* const kUserValueThreeSaveHidden = "threeio.save.hidden";
    constexpr static const char* const kUserValueThreeSaveNormals = "threeio.save.normals";
    constexpr static const char* const kUserValueThreeGeometryType = "threeio.geometry.type";
    constexpr static const char* const kUserValueThreePrecisionEnabled = "threeio.precision.enabled";
    constexpr static const char* const kUserValueThreePrecisionValue = "threeio.precision.value";
    
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