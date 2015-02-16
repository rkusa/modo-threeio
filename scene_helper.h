//
//  scene_helper.h
//  threeio
//
//  Created by Markus Ast on 14.02.15.
//  Copyright (c) 2015 Markus Ast. All rights reserved.
//

#ifndef __threeio__scene_helper__
#define __threeio__scene_helper__

#include <vector>
#include <map>

#include <lxu_scene.hpp>
#include <lxu_math.hpp>
#include <lxitem.h>

class SceneHelper {
public:
//    SceneHelper();
    
    void BuildAxisOrder(const int, int[3]);
    bool CalculateTransform(CLxUser_Item&, LXtMatrix4&);
    bool CalculateLocalTransform(CLxUser_Item&, LXtMatrix4&);
    
private:
    
    void ChanXform(CLxUser_Item&, const char*, LXtVector);
    
    enum
    {
        kRotationOrderXYZ,
        kRotationOrderXZY,
        kRotationOrderYXZ,
        kRotationOrderYZX,
        kRotationOrderZXY,
        kRotationOrderZYX
    };
    
    const char* const kChannelX = ".X";
    const char* const kChannelY = ".Y";
    const char* const kChannelZ = ".Z";
    
    class Types
    {
    public:
        Types();
        
        LXtItemType rotation;
        LXtItemType scale;
        LXtItemType translation;
        LXtItemType transform;
        
    private:
        CLxUser_SceneService	 scene_service;
    };
    
    Types types;
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
        double delta;
        
        for (unsigned i = 0; i < 3; ++i) {
            if ((delta = vector_[i] - rhs[i])) {
                return (delta > 0.0f);
            }
        }
        
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
        
        if (normal_ < rhs.position()) {
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

#endif /* defined(__threeio__scene_helper__) */
