//
//  types.h
//  threeio
//
//  Created by Markus Ast on 16.02.15.
//  Copyright (c) 2015 Markus Ast. All rights reserved.
//

#ifndef __threeio__types__
#define __threeio__types__

#include <vector>
#include <map>

#include <lx_mesh.hpp>
#include <lx_visitor.hpp>

struct Vector2
{
    Vector2(float x, float y) : x(x), y(y) {
    }
    
    Vector2(float v[2]) : x(v[0]), y(v[1]) {
    }
    
    bool operator==(const Vector2& rhs)
    {
        return (x == rhs.x &&
                y == rhs.y);
    }
    
    bool operator!=(const Vector2& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vector2& rhs) const
    {
        if (x > rhs.x) { return false; }
        if (x < rhs.x) { return true; }
        
        if (y > rhs.y) { return false; }
        if (y < rhs.y) { return true; }
        
        return false;
    }
    
    const float x, y;
};

struct Vector3
{
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {
    }
    
    Vector3(double v[3]) : x(v[0]), y(v[1]), z(v[2]) {
    }
    
    bool operator==(const Vector3& rhs)
    {
        return (x == rhs.x &&
                y == rhs.y &&
                z == rhs.z);
    }
    
    bool operator!=(const Vector3& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vector3& rhs) const
    {
        if (x > rhs.x) { return false; }
        if (x < rhs.x) { return true; }
        
        if (y > rhs.y) { return false; }
        if (y < rhs.y) { return true; }
        
        if (z > rhs.z) { return false; }
        if (z < rhs.z) { return true; }
        
        return false;
    }
    
    const double x, y, z;
};

struct Vertex {
    Vertex(double p[3], double n[3], float uv[2]) : position_(p), normal_(n), uv_(uv) {
    }
    
    bool operator==(const Vertex& rhs)
    {
        return position_ == rhs.position() &&
               normal_ == rhs.normal() &&
               uv_ == rhs.uv();
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
        
        if (uv_ < rhs.uv()) {
            return true;
        }
        
        return false;
    }
    
    const Vector3 position() const {
        return position_;
    }
    
    const Vector3 normal() const {
        return normal_;
    }
    
    const Vector2 uv() const {
        return uv_;
    }
    
private:
    Vector3 position_;
    Vector3 normal_;
    Vector2 uv_;
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

class MeshMapVisitor : public CLxImpl_AbstractVisitor
{
public:
    MeshMapVisitor(CLxUser_MeshMap *theMeshMap)
    {
        mesh_map_ = theMeshMap;
    }
    
    const std::vector<const std::string> names() const {
        return names_;
    }
    
private:
    CLxUser_MeshMap *mesh_map_;
    std::vector<const std::string> names_;
    
    virtual LxResult Evaluate ()
    {
        const char *name;
        if (LXx_OK (mesh_map_->Name(&name))) {
            names_.push_back(std::string(name));
        }
        
        return LXe_OK;
    }
};

#endif /* defined(__threeio__types__) */
