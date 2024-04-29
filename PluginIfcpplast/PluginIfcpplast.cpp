// PluginIfcpplast.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#include "framework.h"
#include "PluginIfcpplast.h"
#include "BimModelInput.h"
#include "util.h"
#include "md5.h"

#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/model/BuildingObject.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include "LabGeometryConverter.h"
#include <osg/Matrixd>
#include <ifcpp/IFC4X3/include/IfcText.h>
#include <ifcpp/IFC4X3/include/IfcMapConversion.h>
#include <ifcpp/IFC4X3/include/IfcCoordinateReferenceSystem.h>

#include <ifcpp/geometry/ConverterOSG.h>
//#include <ifcpp/geometry/GeometryConverter.h>
#include <ifcpp/IFC4X3/include/IfcRoot.h>
#include <ifcpp/IFC4X3/include/IfcOwnerHistory.h>
#include <ifcpp/IFC4X3/include/IfcGloballyUniqueId.h>

#include "ifcpp_sub_classes.h"

using namespace IFC4X3;

namespace XBSJ {

	struct MapConversion {


		vector<double>					mapCoords;
		string							crs;


	};


	//std::string  UTF8_To_string(const std::string & str)
	//{
	//	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

	//	wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴  
	//	memset(pwBuf, 0, nwLen * 2 + 2);

	//	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);

	//	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	//	char * pBuf = new char[nLen + 1];
	//	memset(pBuf, 0, nLen + 1);

	//	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	//	std::string retStr = pBuf;

	//	delete[]pBuf;
	//	delete[]pwBuf;

	//	pBuf = NULL;
	//	pwBuf = NULL;

	//	return retStr;
	//}



	osg::Matrixd convertMatrixToOSG(const carve::math::Matrix& mat_in)
	{
		return osg::Matrixd(mat_in.m[0][0], mat_in.m[0][1], mat_in.m[0][2], mat_in.m[0][3],
			mat_in.m[1][0], mat_in.m[1][1], mat_in.m[1][2], mat_in.m[1][3],
			mat_in.m[2][0], mat_in.m[2][1], mat_in.m[2][2], mat_in.m[2][3],
			mat_in.m[3][0], mat_in.m[3][1], mat_in.m[3][2], mat_in.m[3][3]);
	}


	void data_from_hexstring(const char *hexstring, size_t length, void *output) {
		unsigned char *buf = (unsigned char *)output;
		unsigned char byte;
		if (length % 2 != 0) {
			throw std::invalid_argument("data_from_hexstring length % 2 != 0");
		}
		for (size_t i = 0; i < length; ++i) {
			switch (hexstring[i]) {
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				byte = (hexstring[i] - 'a' + 10) << 4;
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				byte = (hexstring[i] - 'A' + 10) << 4;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				byte = (hexstring[i] - '0') << 4;
				break;
			default:
				throw std::invalid_argument("data_from_hexstring invalid hexstring");
			}
			++i;
			switch (hexstring[i]) {
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				byte |= hexstring[i] - 'a' + 10;
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				byte |= hexstring[i] - 'A' + 10;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				byte |= hexstring[i] - '0';
				break;
			default:
				throw std::invalid_argument("data_from_hexstring invalid hexstring");
			}
			*buf++ = byte;
		}
	}



	string data_from_hexstring(const char *hexstring, size_t length) {
		if (length % 2 != 0) {
			throw std::invalid_argument("data_from_hexstring length % 2 != 0");
		}
		if (length == 0) {
			return std::string();
		}
		std::string result;
		result.resize(length / 2);
		data_from_hexstring(hexstring, length, &result[0]);
		return result;
	}

	//十六进制字节流转字符串
	string data_from_hexstring(const std::string &hexstring) {
		return data_from_hexstring(hexstring.c_str(), hexstring.size());
	}

	//映射地图转换实体
	MapConversion getIfcMapConversion(shared_ptr<BuildingModel> m_ifc_model) {

		MapConversion mapcon;

		vector<double> mapCoords;
		string crs;
		// 此处高效的方式是，直接改造需要BuildingModel，在里面获取空间坐标
		const std::map<int, shared_ptr<BuildingEntity> >& map_entities = m_ifc_model->getMapIfcEntities();
		LOG(INFO) << "building entity size : " << map_entities.size();
		size_t loopC = 0;
		for (auto it : map_entities)
		{
			loopC++;
			if(loopC % 100000 == 0)
			{
				double irate = (double)loopC / (double)map_entities.size();
				LOG(INFO) << "getIfcMapConversion func rate " << irate;
			}
			shared_ptr<BuildingEntity> entity = it.second;

			shared_ptr<IfcMapConversion> ifc_mapConversion = dynamic_pointer_cast<IfcMapConversion>(entity);
			if (ifc_mapConversion)
			{
				mapCoords.push_back(ifc_mapConversion->m_Eastings->m_value);
				mapCoords.push_back(ifc_mapConversion->m_Northings->m_value);
				mapCoords.push_back(ifc_mapConversion->m_OrthogonalHeight->m_value);
				mapCoords.push_back(ifc_mapConversion->m_XAxisAbscissa->m_value);
				mapCoords.push_back(ifc_mapConversion->m_XAxisOrdinate->m_value);
				mapcon.mapCoords = mapCoords;
				shared_ptr<IfcCoordinateReferenceSystem> m_crs = ifc_mapConversion->m_TargetCRS;
				if (m_crs) {
					//mapcon.crs = wstring2string(m_crs->m_Name->m_value);
					mapcon.crs = m_crs->m_Name->m_value;
				}
				return mapcon;
			}
		}

		return mapcon;
	}


	shared_ptr<ModelTexture> loadTexture(string imgdata, string format, shared_ptr<BimScene> &scene)
	{

		//string imgdata = wstring2string(data);
		//截图最前位0
		/*if (imgdata[0]=='0') {
			imgdata = imgdata.substr(1);
		}*/

		string path = MD5(imgdata).toStr() + "." + format;
		//imgdata = data_from_hexstring(imgdata);

		shared_ptr<ModelTexture> texture = make_shared<ModelTexture>();

		texture->path = path;
		texture->imageData = imgdata;

		return scene->getTexture(texture);
	}



	shared_ptr<ModelMaterial> toMaterial(const shared_ptr<AppearanceData>& appearance, shared_ptr<BimScene> &scene) {

		shared_ptr<ModelMaterial> material = make_shared<ModelMaterial>();

		material->shinniness = appearance->m_shininess;
		material->diffuse.a = appearance->m_transparency;


		material->ambient.r = appearance->m_color_ambient.r();
		material->ambient.g = appearance->m_color_ambient.g();
		material->ambient.b = appearance->m_color_ambient.b();
		material->ambient.a = appearance->m_color_ambient.a();

		material->diffuse.r = appearance->m_color_diffuse.r();
		material->diffuse.g = appearance->m_color_diffuse.g();
		material->diffuse.b = appearance->m_color_diffuse.b();
		//material->diffuse.a = appearance->m_color_diffuse.a();

		material->specular.r = appearance->m_color_specular.r();
		material->specular.g = appearance->m_color_specular.g();
		material->specular.b = appearance->m_color_specular.b();
		material->specular.a = appearance->m_color_specular.a();

		//此处将纹理加载进来
		if (!appearance->m_imgdata.empty()) {
			material->diffuseTexture = loadTexture(appearance->m_imgdata, appearance->m_format, scene);
		}

		return move(material);
	}

	shared_ptr<ModelMaterial> toMaterial(const std::vector<shared_ptr<AppearanceData> >& vec_product_appearances, shared_ptr<BimScene> &scene) {

		for (size_t ii = 0; ii < vec_product_appearances.size(); ++ii)
		{
			const shared_ptr<AppearanceData>& appearance = vec_product_appearances[ii];
			if (!appearance)
			{
				continue;
			}

			if (appearance->m_apply_to_geometry_type == AppearanceData::GEOM_TYPE_SURFACE || appearance->m_apply_to_geometry_type == AppearanceData::GEOM_TYPE_ANY)
			{
				auto mat = toMaterial(appearance, scene);
				if (mat)
					return move(mat);
			}
			else if (appearance->m_apply_to_geometry_type == AppearanceData::GEOM_TYPE_CURVE)
			{

			}
		}

		return nullptr;
	}


	void  fillMesh(shared_ptr<ModelMesh> &labmesh, shared_ptr<carve::mesh::MeshSet<3> >& meshset, osg::Matrixd & matrix) {
		
		const size_t max_num_faces_per_vertex = 10000;
		std::map<carve::mesh::Face<3>*, double> map_face_area;
		std::map<carve::mesh::Face<3>*, double>::iterator it_face_area;
		for (size_t i_mesh = 0; i_mesh < meshset->meshes.size(); ++i_mesh)
		{
			const carve::mesh::Mesh<3>* mesh = meshset->meshes[i_mesh];

			const size_t num_faces = mesh->faces.size();
			for (size_t i_face = 0; i_face != num_faces; ++i_face)
			{
				carve::mesh::Face<3>* face = mesh->faces[i_face];

				auto face_normal = osg::Vec3d(face->plane.N.x, face->plane.N.y, face->plane.N.z);

				face_normal = osg::Matrixd::transform3x3(face_normal, matrix);
				face_normal = osg::Vec3d(face_normal.x(), face_normal.z(), -face_normal.y());
				const size_t n_vertices = face->nVertices();

				carve::mesh::Edge<3>* e = face->edge;

				vector<carve::mesh::Vertex<3>*> vertexs;
				vector<carve::mesh::Vertex<3>*> uvs;
				face->getVertices(vertexs);
				face->getUVs(uvs);

				auto offset = labmesh->vertexes.size();
				//添加所有顶点
				for (auto & v : vertexs) {
					auto vec = osg::Vec3d(v->v.x, v->v.y, v->v.z);
					//转到绝对值
					vec = vec * matrix;

					//保存 交换 y z
					labmesh->vertexes.push_back(osg::Vec3d(vec.x(), vec.z(), -vec.y()));

					carve::mesh::Vertex<3>* vertex = e->vert;
					vec3 intermediate_normal;
					carve::mesh::Edge<3>* e1 = e;// ->rev->next;
					carve::mesh::Face<3>* f1 = e1->face;
					for (size_t i3 = 0; i3 < max_num_faces_per_vertex; ++i3)
					{
						if (!e1->rev)
						{
							break;
						}
						if (!e1->rev->next)
						{
							break;
						}

						vec3 f1_normal = f1->plane.N;
						const double cos_angle = dot(f1_normal, face_normal);
						if (cos_angle > 0)
						{
							const double deviation = std::abs(cos_angle - 1.0);
							if (deviation < 1e-9)
							{
								double weight = 0.0;
								it_face_area = map_face_area.find(f1);
								if (it_face_area != map_face_area.end())
								{
									weight = it_face_area->second;
								}
								intermediate_normal += weight * f1_normal;
							}
						}

						if (!e1->rev)
						{
							// it's an open mesh
							break;
						}

						e1 = e1->rev->next;
						if (!e1)
						{
							break;
						}
						f1 = e1->face;
						if (f1 == face)
						{
							break;
						}
					}
					const double intermediate_normal_length = intermediate_normal.length();
					if (intermediate_normal_length < 0.0000000001)
					{
						intermediate_normal = face_normal;
					}
					else
					{
						// normalize:
						intermediate_normal *= 1.0 / intermediate_normal_length;
					}
					labmesh->normals.push_back(osg::Vec3(intermediate_normal.x, intermediate_normal.y, intermediate_normal.z));
					//labmesh->normals.push_back(face_normal);

					//labmesh->textcoords.push_back(osg::Vec3d(vec.x(), vec.z(), -vec.y()));
				}

				for (auto & v : uvs) {
					//auto vec = osg::Vec3d(v->v.x, v->v.y, v->v.z);

					labmesh->textcoords.push_back(osg::Vec2d(v->v.x, v->v.y));
				}

				if (n_vertices == 3) {
					//是否判断绕向
					labmesh->indices.push_back(offset);
					labmesh->indices.push_back(offset + 1);
					labmesh->indices.push_back(offset + 2);
				}
				else if (n_vertices == 4) {

					labmesh->indices.push_back(offset);
					labmesh->indices.push_back(offset + 1);
					labmesh->indices.push_back(offset + 2);

					labmesh->indices.push_back(offset);
					labmesh->indices.push_back(offset + 2);
					labmesh->indices.push_back(offset + 3);
				}
				else if (n_vertices > 4)
				{
					//对这个面进行三角化
					LOG(WARNING) << "face vertex size > 4";
				}
			}
		}
	}


	shared_ptr<BimElement> loadIfcElement(shared_ptr<BimScene> & scene, shared_ptr<ProductShapeData>& product_shape,
		shared_ptr<GeometrySettings> isetting) {

		shared_ptr<BimElement> labele = make_shared <BimElement>();
		double eps = 1.5e-8;// m_epsCoplanarDistance;
		//材质 
		auto defaultMaterial = toMaterial(product_shape->getAppearances(),scene);
		if (!defaultMaterial)
			defaultMaterial = make_shared<ModelMaterial>();

		defaultMaterial = scene->getMaterial(defaultMaterial);
		//偏移矩阵
		auto trans = convertMatrixToOSG(product_shape->getTransform());


		//遍历所有 Representation
		std::vector<shared_ptr<RepresentationData> >& vec_product_representations = product_shape->m_vec_representations;
		for (size_t ii_representation = 0; ii_representation < vec_product_representations.size(); ++ii_representation)
		{
			const shared_ptr<RepresentationData>& product_representation_data = vec_product_representations[ii_representation];
			if (product_representation_data->m_ifc_representation.expired())
				continue;

			shared_ptr<IfcRepresentation> ifc_representation(product_representation_data->m_ifc_representation);
			const int representation_id = ifc_representation->m_tag;

			//遍历所有 item shape data  创建mesh 
			const std::vector<shared_ptr<ItemShapeData> >& product_items = product_representation_data->m_vec_item_data;

			shared_ptr<ModelMesh> labmesh = nullptr;

			for (size_t i_item = 0; i_item < product_items.size(); ++i_item)
			{
				const shared_ptr<ItemShapeData>& item_shape = product_items[i_item];

				//当前的材质
				auto currentmaterial = defaultMaterial;
				if (item_shape->m_vec_item_appearances.size() > 0)
				{
					auto tmat = toMaterial(item_shape->m_vec_item_appearances,scene);
					if (tmat) {
						currentmaterial = tmat;
					}
				}

				//如果当前材质和上次的材质不一致，那么新建一个mesh,否则依然用当前mesh去收集三角网
				if (!labmesh) {
					labmesh = make_shared<ModelMesh>();
					labmesh->material = currentmaterial;
					labele->meshes.push_back(labmesh);
				}

				if (currentmaterial != labmesh->material) {
					labmesh = make_shared<ModelMesh>();
					labmesh->material = currentmaterial;
					labele->meshes.push_back(labmesh);
				}

				// create shape for open shells
				for (size_t ii = 0; ii < item_shape->m_meshsets_open.size(); ++ii)
				{
					shared_ptr<carve::mesh::MeshSet<3> >& item_meshset = item_shape->m_meshsets_open[ii];
					//CSG_Adapter::retriangulateMeshSet(item_meshset);
					MeshOps::retriangulateMeshSetForBoolOp_carve(item_meshset, true, isetting, 0);

					fillMesh(labmesh, item_meshset, trans);

					carve::geom::aabb<3> bbox = item_meshset->getAABB();

				}
				//create shape for meshsets
				for (size_t ii = 0; ii < item_shape->m_meshsets.size(); ++ii)
				{
					shared_ptr<carve::mesh::MeshSet<3> >& item_meshset = item_shape->m_meshsets[ii];
					//CSG_Adapter::retriangulateMeshSet(item_meshset);
					//MeshOps::retriangulateMeshSetForBoolOp(item_meshset, true, isetting, 0);
					MeshOps::retriangulateMeshSetForBoolOp_carve(item_meshset, true, isetting, 0);
					//MeshOps::retriangulateMeshSetSimple(item_meshset, true, eps, 0);
					fillMesh(labmesh, item_meshset, trans);
				}
				for (size_t ii = 0; ii < item_shape->m_polylines.size(); ++ii)
				{
					//移除mesh
					labele->meshes.remove(labmesh);
				}

				/*
				// create shape for points
				const std::vector<shared_ptr<carve::input::VertexData> >& vertex_points = item_shape->getVertexPoints();
				for (size_t ii = 0; ii < vertex_points.size(); ++ii)
				{
				const shared_ptr<carve::input::VertexData>& pointset_data = vertex_points[ii];
				if (pointset_data && pointset_data->points.size() > 0)
				{

				}
				}

				// create shape for polylines
				for (size_t ii = 0; ii < item_shape->m_polylines.size(); ++ii)
				{
				shared_ptr<carve::input::PolylineSetData>& polyline_data = item_shape->m_polylines[ii];
				if (polyline_data && polyline_data->points.size() > 0) {

				}
				}*/


			}

		}



		//提取对象的属性

		return move(labele);
	}

	class ProgressRepoter : public ProgressHelper {
	public:
		ProgressRepoter(string stage, double step, double percent) :ProgressHelper(stage, step, percent) {

		}

		void report(shared_ptr<StatusCallback::Message> t)
		{
			if (t->m_message_type == StatusCallback::MESSAGE_TYPE_PROGRESS_VALUE)
			{
				this->skip(t->m_progress_value);
			}
			else if (t->m_message_type == StatusCallback::MESSAGE_TYPE_MINOR_WARNING
				|| t->m_message_type == StatusCallback::MESSAGE_TYPE_WARNING)
			{
				LOG(WARNING) << "ifcpp message warning:" << (t->m_message_text);
			}
			else if (t->m_message_type == StatusCallback::MESSAGE_TYPE_ERROR) {
				LOG(ERROR) << "ifcpp message error:" << (t->m_message_text);
			}
			else {
				LOG(INFO) << "ifcpp message INFO:" << (t->m_message_text);
			}
		}

		static void report(void * re, shared_ptr<StatusCallback::Message> t) {
			ProgressRepoter * rep = (ProgressRepoter*)re;
			rep->report(t);
		}
	};

	class PluginIfcpplastReader : public ModelInputReader {
	public:
		~PluginIfcpplastReader() {}

		bool init(json& cinput) override
		{
			return true;
		}

		bool init(json& cinput, const std::vector<std::uint32_t>& ids) override
		{
			auto ccolorRatio = cinput["colorRatio"];
			if (ccolorRatio.is_number()) {
				auto v = ccolorRatio.get<double>();
				if (v > 0 && v < 10) {
					colorRatio = v;
				}
			}

			//从中提取输入文件信息
			auto cinputfile = cinput["file"];
			if (!cinputfile.is_string()) {
				LOG(ERROR) << "input.file config failed";
				return false;
			}
			inputfile = cinputfile.get<string>();
			inputfile = UTF8_To_string(inputfile);

			shell_ids.assign(ids.begin(), ids.end());
			return true;
		}

#if 0
		//使用参数初始化
		bool init(json & cinput, json & idsinput) override
		{
			auto ccolorRatio = cinput["colorRatio"];
			if (ccolorRatio.is_number()) {
				auto v = ccolorRatio.get<double>();
				if (v > 0 && v < 10) {
					colorRatio = v;
				}
			}

			//从中提取输入文件信息
			auto cinputfile = cinput["file"];
			if (!cinputfile.is_string()) {
				LOG(ERROR) << "input.file config failed";
				return false;
			}
			inputfile = cinputfile.get<string>();
			inputfile = UTF8_To_string(inputfile);

			//std::vector<std::int16_t> select_ids;
			if (!idsinput.empty()) {
				for (size_t i = 0; i < idsinput.size(); i++) {
					shell_ids.push_back(idsinput[i].get<int>());
				}
			}
			return true;
		}
#endif

		//获得需要几次读取
		int getNumScene() override
		{
			return 1;
		}

		//part index mean times loadscene, so far it's not clear.
		shared_ptr<BimScene> loadScene(int partindex) override
		{ 
			//1, 构造ifc的读取对象
			shared_ptr<ReaderSTEP> reader = make_shared<ReaderSTEP>();
			m_ifc_model = make_shared<BuildingModel>();

			//2，解析ifc文件
			shared_ptr<ProgressRepoter> repoter = make_shared<ProgressRepoter>("解析文件", 1, 0.2);
			reader->setMessageCallBack(repoter.get(), ProgressRepoter::report);
			reader->loadModelFromFile(inputfile, m_ifc_model);
			repoter.reset();

			//3, 创建ifc几何体
			shared_ptr<LabGeometryConverter> geometry_converter = make_shared<LabGeometryConverter>(m_ifc_model);
			//通过这个配置应该可以构造不同lod的数据模型，我们现在先默认
			auto m_geom_settings = geometry_converter->getGeomSettings();
			//m_geom_settings->setRenderCreaseEdges(false);
			repoter = make_shared<ProgressRepoter>("创建几何体", 1, 0.5);
			geometry_converter->setMessageCallBack(repoter.get(), ProgressRepoter::report);
			geometry_converter->convertGeometry(shell_ids);
			repoter.reset();

			//4, 寻找其中可以显示的数据
			shared_ptr<ProductShapeData> ifc_project_data;
			std::vector<shared_ptr<ProductShapeData> > vec_products;
			auto map_shape_data = geometry_converter->getShapeInputData();
			std::cout << "总实体数量： " << map_shape_data.size() << std::endl;
			std::cout << "外壳实体数量： " << shell_ids.size() << std::endl;
			for (auto it = map_shape_data.begin(); it != map_shape_data.end(); ++it)
			{
				shared_ptr<ProductShapeData> shape_data = it->second;
				weak_ptr<IfcObjectDefinition>& ifc_object_def_weak = shape_data->m_ifc_object_definition;
				if (ifc_object_def_weak.expired())
				{
					continue;
				}
				//寻找场景结构节点
				shared_ptr<IfcObjectDefinition> ifc_object_def(ifc_object_def_weak);
				if (dynamic_pointer_cast<IfcProject>(ifc_object_def))
				{
					ifc_project_data = shape_data;
					continue;
				}
				//存储到列表中
				if (shape_data)
				{
					vec_products.push_back(shape_data);
				}
			}

			//5,遍历所有几何体去处理，构造场景输入数据
			auto scene = make_shared<BimScene>();
			const int num_products = (int)vec_products.size();
			auto taskrepoter = make_shared<ProgressHelper>("转换几何数据", num_products, 0.2);
			size_t icount = 0;

			for (int i = 0; i < num_products; ++i)
			{
				++icount;
				if(icount % 1000 == 0)
				{
					LOG(WARNING) << "current process : " << (double)icount / (double)num_products;
				}
				taskrepoter->skip(i);

				shared_ptr<ProductShapeData>& product_shape = vec_products[i];
				weak_ptr<IfcObjectDefinition>& ifc_object_def_weak = product_shape->m_ifc_object_definition;
				if (ifc_object_def_weak.expired())
				{
					continue;
				}
				shared_ptr<IfcObjectDefinition> ifc_object_def(ifc_object_def_weak);

				if (dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifc_object_def))
				{
					// geometry will be created in method subtractOpenings
					continue;
				}
				shared_ptr<IfcProduct> ifc_product = dynamic_pointer_cast<IfcProduct>(ifc_object_def);
				if (!ifc_product || !ifc_product->m_Representation)
				{
					continue;
				}

				//根据guid进行过滤
				/*if (!(ifc_product->m_tag == 41809 || ifc_product->m_tag == 2500031))
				{
					continue;
				}*/

				//加载对象
				auto element = loadIfcElement(scene, product_shape, m_geom_settings);

				if (element) {

					scene->elements.push_back(element);

					//解析属性
					element->sid = ifc_product->m_tag;
					element->id = newID();
					element->name = newID();
					sid2id[element->sid] = element->id;

					//其他属性
					//std::vector<std::pair<std::string, shared_ptr<BuildingObject> > > attributes;
					//ifc_product->getAttributes(attributes);
					//for (auto & iter : attributes) {

					//	auto t = scene->getParamIndex(iter.first, "string");
					//	if (iter.first == "Name")
					//	{
					//		auto label = dynamic_pointer_cast<IfcLabel>(iter.second);
					//		if (label && label->m_value.size() > 0) {
					//			element->name = (label->m_value);
					//		}
					//		continue;
					//	}

					//	/*BimPropety bp;
					//	bp.paramIndex = t;
					//	if (iter.second)
					//	{
					//		auto text =  dynamic_pointer_cast<IfcText>(iter.second);
					//		if (text && text->m_value.size() > 0) {
					//			bp.value = (text->m_value);
					//		}
					//		else
					//			bp.value = (iter.second);
					//	}
					//		
					//	element->propertes.push_back(bp);*/
					//}

					hasGeometry.insert(element->sid);
				}

			}
			taskrepoter.reset();

			auto mapcon = getIfcMapConversion(m_ifc_model);

			//获取坐标信息
			string crs = mapcon.crs;

			if (!scene->elements.empty() && !crs.empty()) {
				scene->mapCoords = mapcon.mapCoords;
				scene->crs = crs;
				//scene->angles = angles;
				LOG(INFO) << "读取到szifc 坐标系:" << crs << ",基点坐标：" << scene->mapCoords[0] << "," << scene->mapCoords[1] << "," << scene->mapCoords[2] << ",旋转角：" << scene->mapCoords[3] << "," << scene->mapCoords[4];
			}
			return move(scene);
		}

	private:
		shared_ptr<BuildingModel> m_ifc_model;
		string inputfile = "";
		float colorRatio = 1;
		std::set<int> visited;
		std::set<int> hasGeometry;
		std::map<int, std::string> sid2id;
		std::vector<uint32_t> shell_ids;

	};

	class PluginIfcpplast : public ModelInputPlugin
	{
	public:
		PluginIfcpplast() {
			name = "ifcpplast";
		}
		~PluginIfcpplast() {

		}
		virtual bool supportFormat(string & ext) {

			return ext == ".ifc";
		}

		virtual shared_ptr<ModelInputReader> createReader() {

			return make_shared<PluginIfcpplastReader>();
		}

	private:

	};

}

PLUGINIFCPPLAST_API bool initPlugin(void) {

	auto p = make_shared<XBSJ::PluginIfcpplast>();
	XBSJ::ModelInputPlugin::plugins.push_back(p);

	return true;
}

