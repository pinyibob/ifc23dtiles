#pragma once

#include <osgViewer/Viewer>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Geode>
#include <osg/Material>
#include <osgUtil/Optimizer>
#include <osg/ShapeDrawable>
#include <osg/Node>


// struct shell_info{
//     unsigned int* _id = nullptr;
//     unsigned long _size = 0;
// };

extern std::vector<std::uint32_t> select_ids;
 
class shell_id_collector
{
public:
    shell_id_collector();
    ~shell_id_collector();
    //shell_info execute(const char* file);   
    void execute(const char* file);   
    void execute2(const char* file);   
    osg::ref_ptr<osg::Node> read_ifc(const char*);

private:
    osg::Node* createScene(osg::Node* subgraph, unsigned int tex_width, unsigned int tex_height);
    //osg::ComputeBoundsVisitor boundsVist1;
    osg::Group* _root = nullptr;
};
