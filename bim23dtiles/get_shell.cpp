#include "get_shell.h"
#include "LabConverterOSG.h"

#include <nlohmann/json.hpp>

#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include <ifcpp/geometry/GeomUtils.h>
#include <ifcpp/geometry/SceneGraphUtils.h>
#include <ifcpp/geometry/GeometryConverter.h>
#include <carve/carve.hpp>

#include <thread>
#include <mutex>
#include <regex>
#include <condition_variable>

std::mutex gm;
std::condition_variable cv;

static const char *vertexShader = {
	"varying vec4 color;\n"
	"void main(void)\n"
	"{\n"
	"    color = gl_Vertex;\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"}\n"
};

static const char *fsShader = {
	"varying vec4 color;\n"
	"uniform vec3 pickID;\n"
	"void main(void)\n"
	"{\n"
	"    gl_FragColor = vec4( pickID,1.0 );\n"
	"}\n"
};

class VisitorNodePath : public osg::NodeVisitor
{
public:
	VisitorNodePath() :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}
	
	void apply(osg::Geode &node) override
	{
        using std::string;
        using std::regex;

		std::uint32_t entityid;
        entityid = std::atoi(node.getName().c_str());

		const osg::BoundingSphere& bs = node.getBound();
		osg::Vec3 point = bs.center();// *osg::computeLocalToWorld(node.getParentalNodePaths()[0]);
		if (abs(point.x() >1000.0) || abs(point.y() > 1000.0) || abs(point.z() > 1000.0) || abs(bs.radius()==0)) {
			node.setNodeMask(0x0);
		}

		//std::cout << "node: " << node.getName() << std::endl;
		//判断是否为space,设置透明或不渲染
		//node.setNodeMask(0x0);node.getNumParents()
		for (int i = 0; i < node.getNumDrawables(); ++i)
		{
			osg::Geometry * polyGeom = dynamic_cast<osg::Geometry*>(node.getDrawable(i));
			if (!polyGeom)
                continue;
			//std::cout << "polyGeom: " << polyGeom->getName() << std::endl;
			//polyGeom->setNodeMask(0x0);
			//continue;

			osg::PrimitiveSet* primSet = polyGeom->getPrimitiveSet(0);
			if (primSet->getType() == osg::PrimitiveSet::Mode::POINTS)
			{
				polyGeom->setNodeMask(0x0);
				// This is a PointGeometry
				std::cout << "This is a PointGeometry:" << std::endl;
				continue;
			}
			
			//continue;
			//std::cout << "polyGeom: " << polyGeom->getName() << std::endl;
			
            //ifcspace不加载进来即可
			// string::size_type idx = node.getName().find("IfcSpace");
			// if (idx != string::npos) {
			// 	polyGeom->setNodeMask(0x0);
			// 	continue;
			// }

			/*if (entityid== 299559) {
				std::cout << "R: " << (float)((entityid & 0xff0000) >> 16) / 255 << ",G: " << (float)((entityid & 0xff00) >> 8) / 255 << ",B: " << (float)(entityid & 0xff) / 255 << std::endl;
			}*/
			
			polyGeom->getOrCreateStateSet()->setGlobalDefaults();
			//此处设置shader
			//创建shader
			osg::ref_ptr<osg::Shader> vs1 = new osg::Shader(osg::Shader::VERTEX, vertexShader);
			osg::ref_ptr<osg::Shader> fs1 = new osg::Shader(osg::Shader::FRAGMENT, fsShader);
			//关联shader
			osg::ref_ptr<osg::Program> program1 = new osg::Program;
			program1->addShader(vs1);
			program1->addShader(fs1);
			//添加shader
			polyGeom->getOrCreateStateSet()->setAttributeAndModes(program1, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);


			//设置shader中的uniform变量
			osg::Uniform* pickIDUniform = new osg::Uniform("pickID", osg::Vec3((float)((entityid & 0xff0000) >> 16) / 255, (float)((entityid & 0xff00) >> 8) / 255, (float)(entityid & 0xff) / 255));
			polyGeom->getOrCreateStateSet()->addUniform(pickIDUniform, osg::StateAttribute::Values::ON);

		}
		traverse(node);
	}
	
};

std::vector<std::uint32_t> select_ids;
struct MyCameraPostDrawCallback : public osg::Camera::DrawCallback
{
	MyCameraPostDrawCallback(osg::Image* image, int flag) :
		_image(image),
		_count(flag)
	{
		//std::cout << "---" << _count << "开始筛选构件---" << std::endl;
	}

	void operator () (const osg::Camera& /*camera*/) const override
	{
		static int icount = 0;//控制文件只写入一次
        nlohmann::json ids = nlohmann::json::array();

		if (icount != _count)
            return;

		if (_image && _image->getPixelFormat() == GL_RGBA && _image->getDataType() == GL_FLOAT)
		{
			unsigned int n = select_ids.size();
			for (int row = 0; row < _image->t(); ++row)
			{
				float* data = (float*)_image->data(0, row);
				unsigned char* ptr = _image->data(0, row);
				for (int column = 0; column < _image->s(); ++column)
				{
					float r = float(*data++);
					float g = float(*data++);
					float b = float(*data++);
					float a = float(*data++);

					/*float r = float(*ptr++);
					float g = float(*ptr++);
					float b = float(*ptr++);
					float a = float(*ptr++);*/
					
					uint32_t id = (((int(r * 256.0) | 0) << 16) | ((int(g * 256.0) | 0) << 8) | (int(b * 256.0) | 0));
					int nCount = std::count(select_ids.begin(), select_ids.end(), id);
					//std::cout << "ID: " << id << std::endl;
					/*if (id != 0) {
						std::cout << "ID: " << id << "----" << "R: " << r << ",G:" << g << ",B:" << b << ",A:" << a << std::endl;
					}*/
					
					if (id != 0 && nCount < 1 && a==1.0) {
						select_ids.push_back(id);
						ids.push_back(id);
						//std::cout << "ID: " << id << std::endl;
						//std::cout << "R: " << r << ",G:" << g << ",B:" << b << ",A:" << a << std::endl;
					}
				}
			}
			std::cout << _count << "筛选出外壳构件数：" << select_ids.size() - n << std::endl;
			/*osg::ref_ptr<osg::Image> image = new osg::Image;
			image->readPixels(0, 0, 256, 256,
				GL_RGBA, GL_UNSIGNED_BYTE);

			if (osgDB::writeImageFile(*_image, "autocapture" + std::to_string(_count) + ".png"))
			{
				osg::notify(osg::NOTICE) << "Saved screen image to `" << std::to_string(_count) + ".png" << "`" << std::endl;
			}*/

		}
		
		if (_count == 6 - 1) {
			std::cout << "总外壳构件数：" << select_ids.size() << std::endl;
            cv.notify_all();
        }
		// 	else {
		// 		std::ofstream(outDir+"//"+ filename+".json") << ids;
		// 		std::cout << "json文件路径：" << outDir << std::endl;
		// 	}
		// 	endTime = clock();//计时结束
		// 	cout << "The run time is: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
		// }
		icount++;
	}

	osg::Image* _image = nullptr;
	int _count = 0;
};

osg::Node* createSphere(osg::Vec3 center, float r)
{
	osg::Geode* gnode = new osg::Geode;
	gnode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(center, r)));
	return gnode;
}

shell_id_collector::shell_id_collector()
{
    _root = new osg::Group;
}

shell_id_collector::~shell_id_collector()
{
    //delete _root;
}

osg::Node* shell_id_collector::createScene(osg::Node* subgraph, 
    unsigned int tex_width, unsigned int tex_height)
{
    //场景数据
	if (!subgraph) return 0;

	const osg::BoundingSphere& bs = subgraph->getBound();
	if (!bs.valid())
	{
		return subgraph;
	}
	double viewDistance = 2.0 * bs.radius();
	double znear = (viewDistance - bs.radius())*0.8;
	double zfar = (viewDistance + bs.radius())*1.2;
	float top = bs.radius();
	float right = bs.radius();
	float bottom = top;
	float left = right;


	osg::Vec3 viewUp = osg::Vec3(0.0f, 0.0f, 1.0f);
	osg::Vec3 center = bs.center();
	std::vector<osg::Vec3> eyes;

	std::vector<osg::Vec3> ups;

	//不同视图相机(眼睛)的位置
	//前后视图 -y -> y
	osg::Vec3 eye1 = bs.center() - osg::Vec3(0.0f, 1.0f, 0.0f)*viewDistance;
	osg::Vec3 eye2 = bs.center() + osg::Vec3(0.0f, 1.0f, 0.0f)*viewDistance;

	//左右视图 -x -> x
	osg::Vec3 eye3 = bs.center() - osg::Vec3(1.0f, 0.0f, 0.0f)*viewDistance;
	osg::Vec3 eye4 = bs.center() + osg::Vec3(1.0f, 0.0f, 0.0f)*viewDistance;

	//上下视图 -z -> z
	osg::Vec3 eye5 = bs.center() - osg::Vec3(0.0f, 0.0f, 1.0f)*viewDistance;
	osg::Vec3 eye6 = bs.center() + osg::Vec3(0.0f, 0.0f, 1.0f)*viewDistance;

	eyes.push_back(eye1);
	eyes.push_back(eye2);
	eyes.push_back(eye3);
	eyes.push_back(eye4);
	eyes.push_back(eye5);//此项针对szifc需屏蔽掉
	eyes.push_back(eye6);

	//模型的方向存在一定偏差--对前后左右视图的定义也就不一样

	ups.push_back(viewUp);
	ups.push_back(viewUp);
	ups.push_back(viewUp);
	ups.push_back(viewUp);
	ups.push_back(osg::Vec3(0.0f, 1.0f, 0.0f));
	ups.push_back(osg::Vec3(0.0f, 1.0f, 0.0f));

	//NUMIMG = eyes.size();

	// create a group to contain the quad and the pre render camera.
	osg::Group* parent = new osg::Group;

	for (int i = 0; i < eyes.size(); i++) {

		osg::Camera* camera = new osg::Camera;

		// set up the background color and clear mask.
		camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		_root->addChild(createSphere(eyes[i], 1.0));
		// set up projection.
		//camera->setProjectionMatrixAsFrustum(-proj_right, proj_right, -proj_top, proj_top, znear, zfar);
		camera->setProjectionMatrixAsOrtho(-left, right, -bottom, top, znear, zfar);//设置正投影

		// set view
		camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

		//设置不同的矩阵
		camera->setViewMatrixAsLookAt(eyes[i], bs.center(), ups[i]);

		// set viewport
		camera->setViewport(0, 0, tex_width, tex_height);

		// set the camera to render before the main camera.
		camera->setRenderOrder(osg::Camera::PRE_RENDER);

		//camera->getOrCreateStateSet()->setGlobalDefaults();

		//// tell the camera to use OpenGL frame buffer object where supported.
		camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);//在支持的情况下使用帧缓存

		osg::Texture2D* texture2D = new osg::Texture2D;
		texture2D->setTextureSize(tex_width, tex_height);
		texture2D->setInternalFormat(GL_RGBA);

		texture2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
		texture2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

		camera->attach(osg::Camera::COLOR_BUFFER, texture2D);

		//camera->getOrCreateStateSet()->setAttribute(new osg::Disablei(GL_BLEND, 1));

		//	
		osg::Image* img = new osg::Image;
		img->allocateImage(tex_width, tex_height, 1, GL_RGBA, GL_FLOAT);
		//img->allocateImage(tex_width, tex_height, 1, GL_RGBA, GL_UNSIGNED_BYTE);

		camera->attach(osg::Camera::COLOR_BUFFER, img);//关键语句，将帧缓存输出到Img中

		camera->setPostDrawCallback(new MyCameraPostDrawCallback(img, i));

		texture2D->setImage(0, img);

		// add subgraph to render
		camera->addChild(subgraph);

		parent->addChild(camera);
	}

	return parent;
}

class CustomEvent : public osgGA::GUIEventAdapter
{
public:
    CustomEvent() 
      :osgGA::GUIEventAdapter()
    { setEventType(CUSTOM_EVENT_TYPE); }
    static const EventType CUSTOM_EVENT_TYPE;
};

const osgGA::GUIEventAdapter::EventType CustomEvent::CUSTOM_EVENT_TYPE = osgGA::GUIEventAdapter::USER;

class CloseHandler : public osgGA::GUIEventHandler
{
public:
    CloseHandler(osgViewer::Viewer* viewer) : _viewer(viewer) {}
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == CustomEvent::CUSTOM_EVENT_TYPE)
        {
            _viewer->setDone(true); // 设置为true将导致viewer的run方法退出
            return true;
        }
        return false;
    }

private:
    osgViewer::Viewer* _viewer;
};

osgViewer::Viewer* viewerP = nullptr;

void waker_func()
{
    std::unique_lock il(gm);
    cv.wait(il);
    if(viewerP){
        auto eventQueue = viewerP->getEventQueue();
        osgGA::GUIEventAdapter* event = new CustomEvent();
        eventQueue->addEvent(event);
    }
}

osg::ref_ptr<osg::Node> shell_id_collector::read_ifc(const char* fileName)
{
	// 1: create an IFC model and a reader for IFC files in STEP format:
	shared_ptr<BuildingModel> ifc_model(new BuildingModel());
	shared_ptr<ReaderSTEP> step_reader(new ReaderSTEP());
	//step_reader->setMessageCallBack(&mh, &MessageHandler::slotMessageWrapper);

	// 2: load the model:
	step_reader->loadModelFromFile(fileName, ifc_model);
	shared_ptr<GeometryConverter> geometry_converter(new GeometryConverter(ifc_model));
	//geometry_converter->setMessageCallBack(&mh, &MessageHandler::slotMessageWrapper);
	shared_ptr<GeometrySettings> geom_settings = geometry_converter->getGeomSettings();

    // 为什么刚创建出来就被设置回去
	// the number of vertices per circle can be changed here: (default is 14)
	int numVerticesPerCircle = geom_settings->getNumVerticesPerCircle();
	//std::cout << std::endl << "numVerticesPerCircle: " << numVerticesPerCircle << std::endl;
	geom_settings->setNumVerticesPerCircle(numVerticesPerCircle);

	// adjust epsilon for boolean operations
	geometry_converter->setCsgEps(1.5e-9);

	// convert IFC geometry representations to Carve meshes
	//GeomDebugDump::clearMeshsetDump();

	std::cout << "Converting IFC geometry..." << std::endl;

	osg::Timer_t startTick = osg::Timer::instance()->tick();

	geometry_converter->convertGeometry();

	//shared_ptr<CARVE_FAIL> converter_osg(new ConverterOSG(geom_settings));
	shared_ptr<ConverterOSG> converter_osg(new ConverterOSG(geom_settings));

	converter_osg->setRenderCreaseEdges(false);

	osg::ref_ptr<osg::Switch> model_switch = new osg::Switch();

	converter_osg->convertToOSG(geometry_converter->getShapeInputData(), model_switch);

	osg::Timer_t endTick = osg::Timer::instance()->tick();

	std::cout << "Converted in " << osg::Timer::instance()->delta_s(startTick, endTick) << "s" << std::endl;

	// 3: get a flat map of all loaded IFC entities with geometry:
	const std::map<std::string, shared_ptr<ProductShapeData> >& map_entities = geometry_converter->getShapeInputData();

	std::cout << "map_entities: " << map_entities.size() << std::endl;
    
    return model_switch;
}

void shell_id_collector::execute(const char* filePath)
{
    osgViewer::Viewer viewer;
    viewerP = &viewer;

    std::thread waker(waker_func);

	auto startTime = clock();//计时开始

	osg::ref_ptr<osg::Node> ifc_osg_node = read_ifc(filePath);

	VisitorNodePath vn;
	ifc_osg_node->accept(vn);   

	_root->addChild(ifc_osg_node);
	_root->addChild(createScene(ifc_osg_node, 1024, 1024));

	_root->setName("root");//添加名字方便遍历
	ifc_osg_node->setName("ifc");

	//优化场景数据
	//osgUtil::Optimizer optimizer;
	//optimizer.optimize(_root);

	viewer.setSceneData(_root);
    viewer.addEventHandler(new CloseHandler(&viewer));
	viewer.realize();// 必须调用Realize，否则窗口没有创建
	// osgViewer::GraphicsWindow *pWnd = dynamic_cast<osgViewer::GraphicsWindow*>(viewer.getCamera()->getGraphicsContext());
	// if (pWnd) {
	// 	pWnd->setWindowRectangle(100, 100, 800, 600);

	// 	pWnd->setWindowDecoration(true);
	// }
    viewer.run();
    waker.join();
    // shell_info ires;
    // ires._id = select_ids.data();
    // ires._size = select_ids.size();

	//return std::move(ires);
    return;
}

void shell_id_collector::execute2(const char* filePath)
{
        // 创建一个Viewer对象，它将负责创建窗口和处理事件。
    osgViewer::Viewer viewer;

    // 创建一个Geode，它是一个可以添加到场景图中的节点，用于管理几何体。
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();

	osg::ref_ptr<osg::Node> geode1 = read_ifc(filePath);
	VisitorNodePath vn;
	geode1->accept(vn);   



    // 创建一个ShapeDrawable，它代表了一个立方体。
    osg::ref_ptr<osg::ShapeDrawable> cube = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 0.0f), 1.0f));

    // 设置立方体的颜色为红色。
    cube->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // 将ShapeDrawable添加到Geode中。
    geode->addDrawable(cube);

    // 创建一个Group，它是一个可以包含其他节点的节点。
    osg::ref_ptr<osg::Group> root = new osg::Group();

    // 将Geode添加到Group中。
    root->addChild(geode1);
	root->addChild(createScene(geode1, 1024, 1024));

    // 设置Viewer的根节点。
    viewer.setSceneData(root);

    // 运行Viewer，这将进入事件循环，显示窗口和场景。
    viewer.run();

    return;
}