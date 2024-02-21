#include "cascade_util.h"
#include "time.h"

using namespace std;
using namespace cv;

/********************************************* xml 解析 **********************************************/
void readXmlFile(std::string xml_file, xmlReadWrite &img_info, std::string obj_name_choose)
{
	// XML对象  
	TiXmlDocument mydoc(xml_file.c_str());

	// 加载文档  
	bool loadOk = mydoc.LoadFile();
	if (!loadOk)
	{
		std::cout << "Error: Could not load the xml file." << mydoc.ErrorDesc() << endl;
		exit(1);
	}

	// 根元素
	TiXmlElement *RootElement = mydoc.RootElement();
	TiXmlElement *pEle = RootElement;

	// 遍历该结点  
	for (TiXmlElement *StuElement = pEle->FirstChildElement(); StuElement != NULL; StuElement = StuElement->NextSiblingElement())
	{
		// 图像整体得分(F11)
		if (stricmp(StuElement->Value(), "score") == 0)
		{
			img_info.img_score = stoi(StuElement->FirstChild()->Value());
		}

		// 图像尺寸
		if (stricmp(StuElement->Value(), "size") == 0)
		{
			for (TiXmlElement *sonElement = StuElement->FirstChildElement(); sonElement != NULL; sonElement = sonElement->NextSiblingElement())
			{
				if (stricmp(sonElement->Value(), "width") == 0)  img_info.img_width = stoi(sonElement->FirstChild()->Value());
				if (stricmp(sonElement->Value(), "height") == 0)  img_info.img_hight = stoi(sonElement->FirstChild()->Value());
				if (stricmp(sonElement->Value(), "depth") == 0)  img_info.img_depth = stoi(sonElement->FirstChild()->Value());
			}
		}

		// 标注目标
		if (stricmp(StuElement->Value(), "object") == 0)
		{
			// 获取 GT_box
			cv::Rect gt_rect;

			// 获取 GT_shape
			std::vector<cv::Point> gt_shape;
			cv::Point point_shape;
			xmlobject gt_object_vec;

			bool flag_push = true;

			for (TiXmlElement *sonElement = StuElement->FirstChildElement(); sonElement != NULL; sonElement = sonElement->NextSiblingElement())
			{
				// 判定该目标名称是否为所选目标
				if (stricmp(sonElement->Value(), "name") == 0)
				{
					// 目标名称非空，获取指定box
					if (stricmp(obj_name_choose.c_str(), "") != 0)
					{
						if (stricmp(sonElement->FirstChild()->Value(), obj_name_choose.c_str()) != 0)
						{
							flag_push = false;
							break;
						}
						else
						{
							gt_object_vec.box_name = obj_name_choose;
						}
					}
					// 目标名称为空，获取所有box
					else
					{
						gt_object_vec.box_name = sonElement->FirstChild()->Value();
					}
				}

				// GT_level
				if (stricmp(sonElement->Value(), "level") == 0)
				{
					gt_object_vec.gt_level = stoi(sonElement->FirstChild()->Value());
				}

				// 获取 GT_box
				if (stricmp(sonElement->Value(), "bndbox") == 0)
				{
					/*cv::Rect gt_rect;*/
					for (TiXmlElement *sonElement1 = sonElement->FirstChildElement(); sonElement1 != NULL; sonElement1 = sonElement1->NextSiblingElement())
					{
						if (stricmp(sonElement1->Value(), "xmin") == 0)
						{
							gt_rect.x = stoi(sonElement1->FirstChild()->Value());
						}
						if (stricmp(sonElement1->Value(), "ymin") == 0)
						{
							gt_rect.y = stoi(sonElement1->FirstChild()->Value());
						}
						if (stricmp(sonElement1->Value(), "xmax") == 0)
						{
							gt_rect.width = stoi(sonElement1->FirstChild()->Value()) - gt_rect.x;
						}
						if (stricmp(sonElement1->Value(), "ymax") == 0)
						{
							gt_rect.height = stoi(sonElement1->FirstChild()->Value()) - gt_rect.y;
						}
					}
					gt_object_vec.gt_boxes = gt_rect;
				}

				if (stricmp(sonElement->Value(), "shape") == 0)
				{
					// Refine polygonError 2019.01.17
					TiXmlAttribute *TypeAttribute = sonElement->FirstAttribute();

					// fix bug: type is not the first attribute 2019.04.18
					while (stricmp(TypeAttribute->Name(), "type") != 0)
					{
						TypeAttribute = TypeAttribute->Next();
					}

					if (stricmp(TypeAttribute->Value(), "quad") == 0)
					{
						TiXmlElement *PointElement = sonElement->FirstChildElement();
						int flag_idx = 0;
						for (TiXmlElement *PointSonElement = PointElement->FirstChildElement(); PointSonElement != NULL; PointSonElement = PointSonElement->NextSiblingElement())
						{
							if (flag_idx % 2 == 0)
							{
								point_shape.x = stoi(PointSonElement->FirstChild()->Value());
							}
							else
							{
								point_shape.y = stoi(PointSonElement->FirstChild()->Value());
								gt_shape.push_back(point_shape);
							}
							flag_idx++;
						}
					}
					if (stricmp(TypeAttribute->Value(), "polygon") == 0)
					{
						TiXmlElement *PointElement = sonElement->FirstChildElement();
						int flag_idx = 0;
						for (TiXmlElement *PointSonElement = PointElement->FirstChildElement(); PointSonElement != NULL; PointSonElement = PointSonElement->NextSiblingElement())
						{
							if (flag_idx % 2 == 0)
							{
								point_shape.x = stoi(PointSonElement->FirstChild()->Value());
							}
							else
							{
								point_shape.y = stoi(PointSonElement->FirstChild()->Value());
								gt_shape.push_back(point_shape);
							}
							flag_idx++;
						}
					}
					else if (stricmp(TypeAttribute->Value(), "rect") == 0)
					{
						TiXmlElement *PointElement = sonElement->FirstChildElement();
						int flag_idx = 0;
						std::vector<int> value_rect;
						for (TiXmlElement *PointSonElement = PointElement->FirstChildElement(); PointSonElement != NULL; PointSonElement = PointSonElement->NextSiblingElement())
						{
							value_rect.push_back(stoi(PointSonElement->FirstChild()->Value()));
						}

						// x1y1
						point_shape.x = value_rect[0]; point_shape.y = value_rect[1];
						gt_shape.push_back(point_shape);
						// x2y1
						point_shape.x = value_rect[2]; point_shape.y = value_rect[1];
						gt_shape.push_back(point_shape);
						// x2y2
						point_shape.x = value_rect[2]; point_shape.y = value_rect[3];
						gt_shape.push_back(point_shape);
						// x1y2
						point_shape.x = value_rect[0]; point_shape.y = value_rect[3];
						gt_shape.push_back(point_shape);
						flag_idx++;
					}
					gt_object_vec.gt_shapes = gt_shape;
				}
			}
			if (flag_push)
			{
				img_info.xml_object_vec.push_back(gt_object_vec);
			}
		}
	}
}

void writeXmlFile(std::string xml_file, std::string img_name, xmlReadWrite &img_info)
{
	// XML指针 
	TiXmlDocument *writeDoc = new TiXmlDocument;

	// 文档格式声明  
	// TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	// writeDoc->LinkEndChild(decl);

	// 根元素 
	TiXmlElement *RootElement = new TiXmlElement("annotation");
	writeDoc->LinkEndChild(RootElement);

	// 2.5版本
	TiXmlElement *VersionElement = new TiXmlElement("CreateVersion");
	RootElement->LinkEndChild(VersionElement);
	TiXmlText *VersionContent = new TiXmlText("2.5");
	VersionElement->LinkEndChild(VersionContent);

	// img
	TiXmlElement *ImgElement = new TiXmlElement("filename");
	RootElement->LinkEndChild(ImgElement);
	TiXmlText *ImgContent = new TiXmlText(img_name.c_str());
	ImgElement->LinkEndChild(ImgContent);

	// score
	TiXmlElement *ScoreElement = new TiXmlElement("score");
	RootElement->LinkEndChild(ScoreElement);
	TiXmlText *ScoreContent = new TiXmlText(std::to_string(img_info.img_score).c_str());
	ScoreElement->LinkEndChild(ScoreContent);

	// size
	TiXmlElement *SizeElement = new TiXmlElement("size");
	RootElement->LinkEndChild(SizeElement);

	//size width
	TiXmlElement *WidthElement = new TiXmlElement("width");
	SizeElement->LinkEndChild(WidthElement);
	TiXmlText *WidthContent = new TiXmlText(std::to_string(img_info.img_width).c_str());
	WidthElement->LinkEndChild(WidthContent);

	//size height
	TiXmlElement *HeightElement = new TiXmlElement("height");
	SizeElement->LinkEndChild(HeightElement);
	TiXmlText *HeightContent = new TiXmlText(std::to_string(img_info.img_hight).c_str());
	HeightElement->LinkEndChild(HeightContent);

	//size channel
	TiXmlElement *ChannelElement = new TiXmlElement("depth");
	SizeElement->LinkEndChild(ChannelElement);
	TiXmlText *ChannelContent = new TiXmlText(std::to_string(img_info.img_depth).c_str());
	ChannelElement->LinkEndChild(ChannelContent);

	// segmented
	TiXmlElement *SegmentedElement = new TiXmlElement("segmented");
	RootElement->LinkEndChild(SegmentedElement);
	TiXmlText *SegmentedContent = new TiXmlText("0");
	SegmentedElement->LinkEndChild(SegmentedContent);

	for (int i = 0; i < img_info.xml_object_vec.size(); i++)
	{
		// object
		TiXmlElement *ObjectElement = new TiXmlElement("object");
		RootElement->LinkEndChild(ObjectElement);

		// object name
		TiXmlElement *ObjectNameElement = new TiXmlElement("name");
		ObjectElement->LinkEndChild(ObjectNameElement);
		TiXmlText *ObjectNameContent = new TiXmlText(img_info.xml_object_vec[i].box_name.c_str());
		ObjectNameElement->LinkEndChild(ObjectNameContent);

		// object pose
		TiXmlElement *ObjectPoseElement = new TiXmlElement("pose");
		ObjectElement->LinkEndChild(ObjectPoseElement);
		TiXmlText *ObjectPoseContent = new TiXmlText(img_info.xml_object_vec[i].box_pose.c_str());
		ObjectPoseElement->LinkEndChild(ObjectPoseContent);

		// truncated
		TiXmlElement *TruncatedElement = new TiXmlElement("truncated");
		ObjectElement->LinkEndChild(TruncatedElement);
		TiXmlText *TruncatedContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].box_truncated).c_str());
		TruncatedElement->LinkEndChild(TruncatedContent);

		// difficult
		TiXmlElement *DifficultElement = new TiXmlElement("difficult");
		ObjectElement->LinkEndChild(DifficultElement);
		TiXmlText *DifficultContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].box_difficult).c_str());
		DifficultElement->LinkEndChild(DifficultContent);

		// staintype
		TiXmlElement *StaintypeElement = new TiXmlElement("staintype");
		ObjectElement->LinkEndChild(StaintypeElement);
		TiXmlText *StaintypeContent = new TiXmlText("YW");
		StaintypeElement->LinkEndChild(StaintypeContent);

		// area
		TiXmlElement *AreaElement = new TiXmlElement("area");
		ObjectElement->LinkEndChild(AreaElement);
		TiXmlText *AreaContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].box_area).c_str());
		AreaElement->LinkEndChild(AreaContent);

		// object level
		TiXmlElement *LevelElement = new TiXmlElement("level");
		ObjectElement->LinkEndChild(LevelElement);
		TiXmlText *LevelContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_level).c_str());
		LevelElement->LinkEndChild(LevelContent);

		// object bndbox
		TiXmlElement *BndboxElement = new TiXmlElement("bndbox");
		ObjectElement->LinkEndChild(BndboxElement);

		// object bndbox xmin
		TiXmlElement *XminElement = new TiXmlElement("xmin");
		BndboxElement->LinkEndChild(XminElement);
		TiXmlText *XminContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x).c_str());
		XminElement->LinkEndChild(XminContent);

		// object bndbox ymin
		TiXmlElement *YminElement = new TiXmlElement("ymin");
		BndboxElement->LinkEndChild(YminElement);
		TiXmlText *YminContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y).c_str());
		YminElement->LinkEndChild(YminContent);

		// object bndbox xmax
		TiXmlElement *XmaxElement = new TiXmlElement("xmax");
		BndboxElement->LinkEndChild(XmaxElement);
		TiXmlText *XmaxContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x + img_info.xml_object_vec[i].gt_boxes.width).c_str());
		XmaxElement->LinkEndChild(XmaxContent);

		// object bndbox ymax
		TiXmlElement *YmaxElement = new TiXmlElement("ymax");
		BndboxElement->LinkEndChild(YmaxElement);
		TiXmlText *YmaxContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y + img_info.xml_object_vec[i].gt_boxes.height).c_str());
		YmaxElement->LinkEndChild(YmaxContent);

		// object bndbox
		if (img_info.xml_object_vec[i].gt_shapes.size() > 0)
		{
			// shape
			TiXmlElement *ShapeElement = new TiXmlElement("shape");
			ShapeElement->SetAttribute("type", "quad");
			ShapeElement->SetAttribute("color", "Red");
			ShapeElement->SetAttribute("thickness", "3");
			ObjectElement->LinkEndChild(ShapeElement);

			// points
			TiXmlElement *PointsElement = new TiXmlElement("points");
			ShapeElement->LinkEndChild(PointsElement);

			for (int j = 0; j < img_info.xml_object_vec[i].gt_shapes.size(); j++)
			{
				// object shape x
				TiXmlElement *XElement = new TiXmlElement("x");
				PointsElement->LinkEndChild(XElement);
				TiXmlText *XContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_shapes[j].x).c_str());
				XElement->LinkEndChild(XContent);

				// object shape y
				TiXmlElement *YElement = new TiXmlElement("y");
				PointsElement->LinkEndChild(YElement);
				TiXmlText *YContent = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_shapes[j].y).c_str());
				YElement->LinkEndChild(YContent);
			}
		}
		else
		{
			// shape
			TiXmlElement *ShapeElement = new TiXmlElement("shape");
			ShapeElement->SetAttribute("type", "quad");
			ShapeElement->SetAttribute("color", "Red");
			ShapeElement->SetAttribute("thickness", "3");
			ObjectElement->LinkEndChild(ShapeElement);

			// points
			TiXmlElement *PointsElement = new TiXmlElement("points");
			ShapeElement->LinkEndChild(PointsElement);
			{
				// object shape x
				TiXmlElement *XElement0 = new TiXmlElement("x");
				PointsElement->LinkEndChild(XElement0);
				TiXmlText *XContent0 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x).c_str());
				XElement0->LinkEndChild(XContent0);

				// object shape y
				TiXmlElement *YElement0 = new TiXmlElement("y");
				PointsElement->LinkEndChild(YElement0);
				TiXmlText *YContent0 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y).c_str());
				YElement0->LinkEndChild(YContent0);

				// object shape x
				TiXmlElement *XElement1 = new TiXmlElement("x");
				PointsElement->LinkEndChild(XElement1);
				TiXmlText *XContent1 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x + img_info.xml_object_vec[i].gt_boxes.width).c_str());
				XElement1->LinkEndChild(XContent1);

				// object shape y
				TiXmlElement *YElement1 = new TiXmlElement("y");
				PointsElement->LinkEndChild(YElement1);
				TiXmlText *YContent1 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y).c_str());
				YElement1->LinkEndChild(YContent1);

				// object shape x
				TiXmlElement *XElement2 = new TiXmlElement("x");
				PointsElement->LinkEndChild(XElement2);
				TiXmlText *XContent2 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x + img_info.xml_object_vec[i].gt_boxes.width).c_str());
				XElement2->LinkEndChild(XContent2);

				// object shape y
				TiXmlElement *YElement2 = new TiXmlElement("y");
				PointsElement->LinkEndChild(YElement2);
				TiXmlText *YContent2 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y + img_info.xml_object_vec[i].gt_boxes.height).c_str());
				YElement2->LinkEndChild(YContent2);

				// object shape x
				TiXmlElement *XElement3 = new TiXmlElement("x");
				PointsElement->LinkEndChild(XElement3);
				TiXmlText *XContent3 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.x).c_str());
				XElement3->LinkEndChild(XContent3);

				// object shape y
				TiXmlElement *YElement3 = new TiXmlElement("y");
				PointsElement->LinkEndChild(YElement3);
				TiXmlText *YContent3 = new TiXmlText(std::to_string(img_info.xml_object_vec[i].gt_boxes.y + img_info.xml_object_vec[i].gt_boxes.height).c_str());
				YElement3->LinkEndChild(YContent3);
			}
		}
	}
	writeDoc->SaveFile(xml_file.c_str());
	delete writeDoc;
}

void flipXmlFile(xmlReadWrite &img_info_old, xmlReadWrite &img_info_new)
{
	img_info_new.img_depth = img_info_old.img_depth;
	img_info_new.img_score = img_info_old.img_score;
	img_info_new.img_width = img_info_old.img_width;
	img_info_new.img_hight = img_info_old.img_hight;
	for (auto idx_obj = 0; idx_obj < img_info_old.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = img_info_old.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = img_info_old.xml_object_vec[idx_obj].gt_level;
		newobj.gt_boxes.x = img_info_old.img_width - img_info_old.xml_object_vec[idx_obj].gt_boxes.x - img_info_old.xml_object_vec[idx_obj].gt_boxes.width;
		newobj.gt_boxes.y = img_info_old.xml_object_vec[idx_obj].gt_boxes.y;
		newobj.gt_boxes.width = img_info_old.xml_object_vec[idx_obj].gt_boxes.width;
		newobj.gt_boxes.height = img_info_old.xml_object_vec[idx_obj].gt_boxes.height;
		for (auto idx_shape = 0; idx_shape < img_info_old.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point_flip;
			point_flip.x = img_info_old.img_width - img_info_old.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
			point_flip.y = img_info_old.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
			newobj.gt_shapes.push_back(point_flip);
		}
		img_info_new.xml_object_vec.push_back(newobj);
	}
}

bool checkXmlFile(xmlReadWrite img_info)
{
	if (img_info.xml_object_vec.size() == 0)
	{
		return false;
	}
	for (auto idx_obj = 0; idx_obj < img_info.xml_object_vec.size(); idx_obj++)
	{
		//// 左上角越界
		//if ((img_info.xml_object_vec[idx_obj].gt_boxes.x < 1) || (img_info.xml_object_vec[idx_obj].gt_boxes.y < 1))
		//{
		//	return false;
		//}
		//// 宽高越界
		//if ((img_info.xml_object_vec[idx_obj].gt_boxes.width >= img_info.img_width - 1) || (img_info.xml_object_vec[idx_obj].gt_boxes.height >= img_info.img_hight - 1))
		//{
		//	return false;
		//}
		//if (img_info.xml_object_vec[idx_obj].gt_boxes.width <4 || img_info.xml_object_vec[idx_obj].gt_boxes.height<4)
		//{
		//	return false;
		//}
		//// 右下角越界
		//if (img_info.xml_object_vec[idx_obj].gt_boxes.width + img_info.xml_object_vec[idx_obj].gt_boxes.x > img_info.img_width - 2)
		//{
		//	return false;
		//}
		//if (img_info.xml_object_vec[idx_obj].gt_boxes.height + img_info.xml_object_vec[idx_obj].gt_boxes.y > img_info.img_hight - 2)
		//{
		//	return false;
		//}

		if (img_info.xml_object_vec[idx_obj].gt_shapes.size()==0)
		{
			return false;
		}

		/*for (auto idx_shape = 0; idx_shape < img_info.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point_shape = img_info.xml_object_vec[idx_obj].gt_shapes[idx_shape];
			if ((point_shape.x < 0) || (point_shape.y < 0) || (point_shape.x >= img_info.img_width-1) || (point_shape.y >= img_info.img_hight-1))
			{
				return false;
			}
		}*/
	}
	return true;
}


/********************************************* csv 解析 **********************************************/
void readCsvFile(std::string csv_file, std::vector<std::pair<std::string,xmlReadWrite>> &img_info, std::string obj_name_choose)
{
	ifstream fin(csv_file);
	string line;
	int idx = 0;
	while (getline(fin, line))
	{
		// 跳过第一行
		if (idx==0)
		{
			idx++;
			continue;
		}
		
		// 分割字符串
		string::size_type position1 = line.find(',');
		if (position1 != line.npos)
		{
			// 图像名
			string img_name = line.substr(0, position1);
			string img_box = line.substr(position1 + 1, line.size() - 1);
			
			// box坐标
			std::vector<int> boxes;
			std::istringstream ss(img_box);
			std::string value_box;
			while (std::getline(ss, value_box, ' '))
			{
				boxes.push_back(std::stoi(value_box));
			}	

			xmlobject info;
			info.box_name = obj_name_choose;
			info.gt_boxes = cv::Rect(boxes[0], boxes[1], boxes[2] - boxes[0], boxes[3] - boxes[1]);

			// 信息汇总
			if (img_info.size()==0)
			{
				std::pair<string, xmlReadWrite> obj_new;
				obj_new.first = img_name;
				obj_new.second.xml_object_vec.push_back(info);
				img_info.push_back(obj_new);
			}
			else
			{
				for (int i = 0; i < img_info.size(); i++)
				{
					if (img_name == img_info[i].first)
					{
						img_info[i].second.xml_object_vec.push_back(info);
						break;
					}
					else if (i == img_info.size() - 1)
					{
						std::pair<string, xmlReadWrite> obj_new;
						obj_new.first = img_name;
						obj_new.second.xml_object_vec.push_back(info);
						img_info.push_back(obj_new);
						break;
					}
				}
			}
		}
		idx++;
	}
}

/************************************************** 文件创建 ******************************************************/

void checkName(std::string& name)
{
	std::string::size_type startpos = 0;
	while (startpos != std::string::npos)
	{
		startpos = name.find('\\');
		if (startpos != std::string::npos)
		{
			name.replace(startpos, 1, "/");
		}
	}
}

void createFolder(std::string fodler_path)
{
	checkName(fodler_path);
	std::string folder_create("");
	for (int idx = 0; idx < fodler_path.size(); idx++)
	{
		if (fodler_path[idx] == '/')
		{
			folder_create = fodler_path.substr(0, idx);
			if (_access(folder_create.c_str(), 0) != 0)
			{
				_mkdir(folder_create.c_str());
			}
		}
	}
	if (_access(fodler_path.c_str(), 0) != 0)
	{
		_mkdir(fodler_path.c_str());
	}
}


/************************************************** 文件读取 ******************************************************/

void readFiles(const std::string &path, std::vector<std::string> &files_name, int *files_number)
{
	//文件存储信息结构体 
	struct _finddata_t fileinfo;
	//保存文件句柄 
	intptr_t fHandle;
	//文件数记录器
	int i = 0;

	fHandle = _findfirst(path.c_str(), &fileinfo);

	if (-1L == fHandle)
	{
		std::cout << "no files in " << path << std::endl;
	}
	else
	{
		do
		{
			i++;
			files_name.push_back(fileinfo.name);
		} while (_findnext(fHandle, &fileinfo) == 0);
	}

	_findclose(fHandle);
	*files_number = i;
}

void readDirs(const std::string &path, std::vector<std::string> &dir_name)
{
	_finddata_t file_info;
	string current_path = path + R"(\*.*)";
	int handle = _findfirst(current_path.c_str(), &file_info);
	//返回值为-1则查找失败
	if (-1 == handle)
	{
		cout << "cannot match the path" << endl;
		return;
	}

	do
	{
		//判断是否子目录
		if (file_info.attrib == _A_SUBDIR)
		{
			if (strcmp(file_info.name, "..") != 0 && strcmp(file_info.name, ".") != 0)
			{
				dir_name.push_back(file_info.name);
			}
		}

	} while (!_findnext(handle, &file_info));

	_findclose(handle);
}

void readFilesRecurse(const std::string &path, std::vector<std::string> &files_name, std::vector<std::string> &files_name_full, int layer, std::string img_ext)
{
	_finddata_t file_info;
	string current_path = path + R"(\*.*)"; //也可以用/*来匹配所有
	intptr_t handle = _findfirst(current_path.c_str(), &file_info);
	//返回值为-1则查找失败
	if (-1 == handle)
	{
		cout << "cannot match the path" << endl;
		return;
	}

	do
	{
		//判断是否子目录
		if (file_info.attrib == _A_SUBDIR)
		{
			int layer_tmp = layer;
			if (strcmp(file_info.name, "..") != 0 && strcmp(file_info.name, ".") != 0)
			{
				readFilesRecurse(path + R"(\)" + file_info.name, files_name, files_name_full, layer_tmp + 1, img_ext);
			}
		}
		else
		{
			// 无后缀匹配要求
			if (img_ext == "")
			{
				files_name.push_back(file_info.name);
				files_name_full.push_back(path + R"(\)" + file_info.name);
			}
			else
			{
				// 匹配后缀
				string file_name(file_info.name);
				if (file_name.substr(file_name.size() - 4, file_name.size()) == img_ext)
				{
					files_name.push_back(file_info.name);
					files_name_full.push_back(path + R"(\)" + file_info.name);

				}
			}
		}
	} while (!_findnext(handle, &file_info));  //返回0则遍历完
											   //关闭文件句柄
	_findclose(handle);

}

/************************************************* 图像增广 *****************************************************/

// 获取shapes点边界极值: min_x, min_y, max_x, max_y
std::vector<int> get_Min_Max_Boundary(xmlReadWrite input_xml)
{
	std::vector<int> location_x, location_y;
	for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec.size(); idx_shape++)
	{
		for (auto idx_point = 0; idx_point < input_xml.xml_object_vec[idx_shape].gt_shapes.size(); idx_point++)
		{
			location_x.push_back(input_xml.xml_object_vec[idx_shape].gt_shapes[idx_point].x);
			location_y.push_back(input_xml.xml_object_vec[idx_shape].gt_shapes[idx_point].y);
		}
	}

	std::vector<int>::iterator max_x = std::max_element(location_x.begin(), location_x.end());
	std::vector<int>::iterator min_x = std::min_element(location_x.begin(), location_x.end());
	std::vector<int>::iterator max_y = std::max_element(location_y.begin(), location_y.end());
	std::vector<int>::iterator min_y = std::min_element(location_y.begin(), location_y.end());

	auto shape_x_max = *max_x;
	auto shape_x_min = *min_x;
	auto shape_y_max = *max_y;
	auto shape_y_min = *min_y;

	std::vector<int> output;
	output.push_back(shape_x_min);
	output.push_back(shape_y_min);
	output.push_back(shape_x_max);
	output.push_back(shape_y_max);

	if (shape_x_min > input_xml.img_width - shape_x_max)
	{
		output.push_back(input_xml.img_width - shape_x_max);
	}
	else
	{
		output.push_back(shape_x_min);
	}

	if (shape_y_min > input_xml.img_hight - shape_y_max)
	{
		output.push_back(input_xml.img_hight - shape_y_max);
	}
	else
	{
		output.push_back(shape_y_min);
	}

	return output;
}

std::pair<cv::Mat, xmlReadWrite> imgAug(cv::Mat input_img, xmlReadWrite input_xml, std::string flag_aug)
{
	cv::Mat img_src = input_img.clone();
	if (flag_aug == "flag_ori")
	{
		std::pair<cv::Mat, xmlReadWrite> output;
		output.first = img_src;
		output.second = input_xml;
		return output;
	}
	if (flag_aug == "flag_flip_up")
	{
		return img_Flip(img_src, input_xml, 0);
	}
	if (flag_aug == "flag_flip_lr")
	{
		return img_Flip(img_src, input_xml, 1);
	}
	if (flag_aug == "flag_flip_all")
	{
		return img_Flip(img_src, input_xml, -1);
	}
	if (flag_aug == "flag_noise")
	{
		return img_GaussianNoise(img_src, input_xml);
	}
	if (flag_aug == "flag_blur")
	{
		return img_Blur(img_src, input_xml);
	}
	if (flag_aug == "flag_sharpen")
	{
		return img_Sharpen(img_src, input_xml);
	}
	if (flag_aug == "flag_affine")
	{
		return img_Affine(img_src, input_xml, 0.01, 0.01);
	}
	if (flag_aug == "flag_rotate")
	{
		return img_Rotate(img_src, input_xml, 0, 2);
	}
	if (flag_aug == "flag_translation")
	{
		return img_Translation(img_src, input_xml, 30, 30);
	}
	if (flag_aug == "flag_gamma") 
	{
	
		return img_Gamma(input_img, input_xml, 0.7, 0.9, 0.8, false);
	
	}
	if (flag_aug=="flag_illum")
	{
		return img_Illum_lighting(input_img, input_xml, 100, 110, 100, 150, "gaussian");
	}
}

std::pair<cv::Mat, xmlReadWrite> img_Blur(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u0(0, 2);

	//int size[] = { 3, 5, 7, 9 };
	//int size[] = { 3, 3, 3 };
	int size[] = { 3, 5, 7 };

	int size_index = u0(e1);

	cv::GaussianBlur(input_img, output_img, cv::Size(size[size_index], size[size_index]), 2.0);

	output_xml = input_xml;	
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Blur_Kernel(cv::Mat input_img, xmlReadWrite input_xml, int blur_kernel_size)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	cv::GaussianBlur(input_img, output_img, cv::Size(blur_kernel_size, blur_kernel_size), 2.0);

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Sharpen(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	//创建并初始化滤波模板
	cv::Mat kernel(3, 3, CV_32F, cv::Scalar(0));
	kernel.at<float>(1, 1) = 5.0;
	kernel.at<float>(0, 1) = -1.0;
	kernel.at<float>(1, 0) = -1.0;
	kernel.at<float>(1, 2) = -1.0;
	kernel.at<float>(2, 1) = -1.0;

	output_img = cv::Mat(input_img.size(), input_img.type());

	//对图像进行滤波
	cv::filter2D(input_img, output_img, input_img.depth(), kernel);

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_GaussianNoise(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	output_img = input_img.clone();
	//int Noise_s_value[] = { 3, 4, 5, 6, 7 };
	//int Noise_a = 0;
	//int Noise_b = 4;
	//int Noise_index = (rand() % (Noise_b - Noise_a + 1)) + Noise_a;

	//std::random_device rd;
	//std::mt19937 gen(rd());
	//std::normal_distribution<> d(0, Noise_s_value[Noise_index]);

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u1(3, 7);


	auto rows = output_img.rows;
	auto cols = output_img.cols * output_img.channels();

	for (int i = 0; i < rows; i++)
	{
		auto p = output_img.ptr<uchar>(i);
		for (int j = 0; j < cols; j++)
		{
			auto tmp = p[j] + u1(e1);
			tmp = tmp > 255 ? 255 : tmp;
			tmp = tmp < 0 ? 0 : tmp;
			p[j] = tmp;
		}
	}

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Gamma(cv::Mat input_img, xmlReadWrite input_xml, float gamma_factor_min, float gamma_factor_max, float famma_factor_fix, bool flag_fix)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	float value_gamma = 0;

	if (flag_fix)
	{
		value_gamma = famma_factor_fix;
	}
	else
	{
		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u1(gamma_factor_min * 100, gamma_factor_max * 100);

		value_gamma = u1(e1) / 100.0;
	}


	// 建表
	unsigned char lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), value_gamma)*255.0f);
	}

	output_img = input_img.clone();
	const int img_channels = output_img.channels();

	switch (img_channels)
	{
		case 1:
		{
			cv::MatIterator_<uchar> it, end;
			for (it = output_img.begin<uchar>(), end = output_img.end<uchar>(); it != end; it++)
			{
				*it = lut[(*it)];
			}
			break;
		}
		case 3:
		{
			cv::MatIterator_<Vec3b> it3, end3;
			for (it3 = output_img.begin<Vec3b>(), end3 = output_img.end<Vec3b>(); it3 != end3; it3++)
			{
				(*it3)[0] = lut[((*it3)[0])];
				(*it3)[1] = lut[((*it3)[1])];
				(*it3)[2] = lut[((*it3)[2])];
			}
			break;
		}
		default:
		{
			std::cout << "img channel is wrong! plz check!" << std::endl;
			break;
		}
	}

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_LogAug(cv::Mat input_img, xmlReadWrite input_xml, float param1, float param2)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	if (input_img.channels()<3)
	{
		output_img = cv::Mat(input_img.size(), CV_32FC1);
	}
	else
	{
		output_img = cv::Mat(input_img.size(), CV_32FC3);
	}

	float *logtable = new float[256];
	for (int i = 0; i < 256; i++)
	{
		logtable[i] = log((float)i *param1 + param2);
	}
	for (int i = 0; i < input_img.rows; i++)
	{
		uchar * ptrg = input_img.ptr<uchar>(i);
		float * ptr = output_img.ptr<float>(i);
		for (int j = 0; j < output_img.cols*input_img.channels(); j++)
		{
			ptr[j] = logtable[ptrg[j]];
		}
	}
	delete[] logtable;
	normalize(output_img, output_img, 0, 255, CV_MINMAX);
	
	//转换成8bit图像显示    
	convertScaleAbs(output_img, output_img);

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_ContrastAug(cv::Mat input_img, xmlReadWrite input_xml, float alpha, float beta, bool flag_fix)
{
	/*
	m     – 目标矩阵。如果m在运算前没有合适的尺寸或类型，将被重新分配。

	rtype – 目标矩阵的类型。因为目标矩阵的通道数与源矩阵一样，所以rtype也可以看做是目标矩阵的位深度。如果rtype为负值，目标矩阵和源矩阵将使用同样的类型。

	alpha – 尺度变换因子（可选）。

	beta  – 附加到尺度变换后的值上的偏移量（可选）。
	"*/

	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	float value_alpha = 0;
	float value_bate = 0;

	if (flag_fix)
	{
		value_alpha = alpha;
		value_bate = beta;
	}
	else
	{
		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u1(0, 9);

		value_alpha = alpha + u1(e1) / 100.0;
		value_bate = beta + u1(e1) / 100.0;
	}

	input_img.convertTo(output_img, -1, value_alpha, value_bate);// 参数可调

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

cv::Mat illum_Get_Distance(cv::Mat Img_src, cv::Point point)
{
	int im_w = Img_src.cols;
	int im_h = Img_src.rows;
	int im_c = Img_src.channels();

	if (point.x<0 || point.x>im_w || point.y <0 || point.y>im_h)
	{
		// https://blog.csdn.net/u014571489/article/details/82258467
		point.x = rand() % (im_w + 1);//[0 - im_w]
		point.y = rand() % (im_h + 1);//[0 - im_h]
	}

	cv::Mat tmp_distance = cv::Mat::zeros(cv::Size(im_w, im_h), CV_32FC1);
	for (size_t i = 0; i < im_h; i++)
	{
		for (size_t j = 0; j < im_w; j++)
		{
			tmp_distance.at<float>(i, j) = sqrt(pow((1.0*i - point.y), 2) + pow((1.0*j - point.x), 2));
		}
	}

	return tmp_distance;
}
cv::Mat illum_Get_Guassian_Map(cv::Mat Img_distance, float value_power, float value_scale, int value_width, int value_height)
{
	int value_radius = int(std::sqrt(std::pow(value_width, 2) + std::pow(value_height, 2)));
	cv::Mat Guassian_Map;
	cv::exp(-0.5 / (value_radius*value_scale)*Img_distance, Guassian_Map);
	return Guassian_Map*value_power;
}
cv::Mat illum_Get_Linear_Map(cv::Mat Img_distance, float value_power, float value_scale, int value_width, int value_height)
{
	int value_radius = int(std::sqrt(std::pow(value_width, 2) + std::pow(value_height, 2)));
	cv::Mat Linear_Map = (-1 / (std::exp(value_scale))*Img_distance + value_radius - 1)*value_power / value_radius;
	return Linear_Map;
}

std::pair<cv::Mat, xmlReadWrite> img_Illum_lighting(cv::Mat input_img, xmlReadWrite input_xml, float power_min, float power_max, float scale_min, float scale_max, std::string light_type)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	int im_w = input_img.cols;
	int im_h = input_img.rows;
	int im_c = input_img.channels();

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(-50, 50);
	uniform_int_distribution<unsigned> u1(power_min, power_max);
	uniform_int_distribution<unsigned> u2(scale_min, scale_max);


	int center_x = 0.5*input_img.cols;
	int center_y = 0.5*input_img.rows;

	cv::Point value_center(center_x + u0(e1), center_y + u0(e1));

	cv::Mat res_map;
	cv::Mat img_distance = illum_Get_Distance(input_img, value_center);

	if (light_type == "gaussian")
	{
		res_map = illum_Get_Guassian_Map(img_distance, u1(e1)/100.0, u2(e1)/100.0, im_w, im_h);
	}
	else if (light_type == "linear")
	{
		res_map = illum_Get_Linear_Map(img_distance, u1(e1)/100.0, u2(e1)/100.0, im_w, im_h);
	}
	else
	{
		std::cout << "Light_type Error!" << std::endl;
		exit(1);
	}

	cv::Mat img_res_map;
	cv::Mat Img_src_float;

	if (im_c == 3)
	{
		std::vector<cv::Mat> img_channels;
		img_channels.push_back(res_map);
		img_channels.push_back(res_map);
		img_channels.push_back(res_map);
		cv::merge(img_channels, img_res_map);
		input_img.convertTo(Img_src_float, CV_32FC3, 1, 0);
	}
	else
	{
		img_res_map = res_map.clone();
		input_img.convertTo(Img_src_float, CV_32FC1, 1, 0);

	}

	cv::Mat res_img = Img_src_float.mul(img_res_map);

	if (im_c == 3)
	{
		res_img.convertTo(output_img, CV_8UC3, 1, 0);
	}
	else
	{
		res_img.convertTo(output_img, CV_8UC1, 1, 0);
	}

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_HsvJitter(cv::Mat input_img, xmlReadWrite input_xml, float hue, float saturation, float exposure)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	// 参考darknet框架中的image.c实现
	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<signed> u0(0, 100);
	uniform_int_distribution<signed> u1(-hue*100, hue*100);
	uniform_int_distribution<signed> u2(saturation*100,100);
	uniform_int_distribution<signed> u3(exposure*100, 100);

	float value_hue = u1(e1);
	float value_sat, value_exp;
	if (u0(e1) % 2)
	{
		value_sat = u2(e1)/100.0;
	}
	else
	{
		value_sat = 100. / u2(e1);
	}
	if (u0(e1) % 2)
	{
		value_exp = u3(e1)/100.0;
	}
	else
	{
		value_exp = 100. / u3(e1);
	}

	cv::Mat img_hsv;
	cv::cvtColor(input_img, img_hsv, CV_BGR2HSV);

	std::vector<cv::Mat> channels_hsv, channels_hsv_refine;
	cv::split(img_hsv, channels_hsv);

	cv::Mat hsv_H, hsv_S, hsv_V;
	hsv_H = channels_hsv.at(0);
	hsv_S = channels_hsv.at(1);
	hsv_V = channels_hsv.at(2);

	hsv_H = hsv_H + value_hue;
	hsv_S = hsv_S*value_sat;
	hsv_V = hsv_V*value_exp;

	channels_hsv_refine.push_back(hsv_H);
	channels_hsv_refine.push_back(hsv_S);
	channels_hsv_refine.push_back(hsv_V);

	cv::merge(channels_hsv_refine, output_img);

	cv::cvtColor(output_img, output_img, CV_HSV2BGR);

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite>  img_ScaleJitter(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	output = img_Crop(input_img, input_xml);
	cv::resize(output.first, output_img, input_img.size(), 0, 0, cv::INTER_AREA);

	output.first = output_img;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_PCAJitter(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<signed> u0(0, 100);


	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_ChannelShift(cv::Mat input_img, xmlReadWrite input_xml)
{
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	if (input_img.channels() == 3)
	{
		std::vector<cv::Mat> img_channels(3);
		cv::split(input_img, img_channels);
		std::random_shuffle(img_channels.begin(), img_channels.end());
		cv::merge(img_channels, output_img);	
	}
	else
	{
		output_img = input_img;
	}

	output_xml = input_xml;
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Flip(cv::Mat input_img, xmlReadWrite input_xml, int flag_flip)
{
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	cv::flip(input_img, output_img, flag_flip);

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	// 水平
	if (flag_flip > 0)
	{
		for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
		{
			xmlobject newobj;
			newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
			newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
			newobj.gt_boxes.x = input_xml.img_width - input_xml.xml_object_vec[idx_obj].gt_boxes.x - input_xml.xml_object_vec[idx_obj].gt_boxes.width;
			newobj.gt_boxes.y = input_xml.xml_object_vec[idx_obj].gt_boxes.y;
			newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width;
			newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height;
			for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
			{
				cv::Point point;
				point.x = input_xml.img_width - input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
				point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
				newobj.gt_shapes.push_back(point);
			}
			output_xml.xml_object_vec.push_back(newobj);
		}
	}
	// 垂直
	else if (flag_flip == 0)
	{
		for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
		{
			xmlobject newobj;
			newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
			newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
			newobj.gt_boxes.x = input_xml.xml_object_vec[idx_obj].gt_boxes.x;
			newobj.gt_boxes.y = input_xml.img_hight - input_xml.xml_object_vec[idx_obj].gt_boxes.y - input_xml.xml_object_vec[idx_obj].gt_boxes.height;
			newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width;
			newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height;
			for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
			{
				cv::Point point;
				point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
				point.y = input_xml.img_hight - input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
				newobj.gt_shapes.push_back(point);
			}
			output_xml.xml_object_vec.push_back(newobj);
		}
	}
	// 同时
	else
	{
		for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
		{
			xmlobject newobj;
			newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
			newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
			newobj.gt_boxes.x = input_xml.img_width - input_xml.xml_object_vec[idx_obj].gt_boxes.x - input_xml.xml_object_vec[idx_obj].gt_boxes.width;
			newobj.gt_boxes.y = input_xml.img_hight - input_xml.xml_object_vec[idx_obj].gt_boxes.y - input_xml.xml_object_vec[idx_obj].gt_boxes.height;
			newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width;
			newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height;
			for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
			{
				cv::Point point;
				point.x = input_xml.img_width - input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
				point.y = input_xml.img_hight - input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
				newobj.gt_shapes.push_back(point);
			}
			output_xml.xml_object_vec.push_back(newobj);
		}
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Translation(cv::Mat input_img, xmlReadWrite input_xml, int translation_x, int translation_y)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	// 获取shape边界
	std::vector<int> shape_boundary = get_Min_Max_Boundary(input_xml);
	int min_x = shape_boundary[4] - 1 > translation_x ? translation_x : shape_boundary[4] - 1;
	int min_y = shape_boundary[5] - 1 > translation_y ? translation_y : shape_boundary[5] - 1;

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(0, min_x);
	uniform_int_distribution<unsigned> u1(0, min_y);

	int trans_x = u0(e1);
	int trans_y = u1(e1);

	cv::Mat t_mat = cv::Mat::zeros(2, 3, CV_32FC1);
	t_mat.at<float>(0, 0) = 1;
	t_mat.at<float>(0, 2) = trans_x; //水平平移量  
	t_mat.at<float>(1, 1) = 1;
	t_mat.at<float>(1, 2) = trans_y; //竖直平移量

	cv::warpAffine(input_img, output_img, t_mat, input_img.size());

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
		
		int location_x1 = input_xml.xml_object_vec[idx_obj].gt_boxes.x + trans_x;
		int location_y1 = input_xml.xml_object_vec[idx_obj].gt_boxes.y + trans_y;
		int location_x2 = input_xml.xml_object_vec[idx_obj].gt_boxes.width + location_x1;
		int location_y2 = input_xml.xml_object_vec[idx_obj].gt_boxes.height + location_y1;

		// 修正box
		location_x1 = location_x1 > 0 ? location_x1 : 0;
		location_y1 = location_y1 > 0 ? location_y1 : 0;
		location_x2 = location_x2 < input_xml.img_width ? location_x2 : input_xml.img_width - 1;
		location_y2 = location_y2 < input_xml.img_hight ? location_y2 : input_xml.img_hight - 1;

		// TODO: 这里还有其他情况需要判定，因为平移距离小，所以暂时偷懒不写了
		if ((location_x1>=location_x2)||(location_y1 >= location_y2))
		{
			continue;
		}
		newobj.gt_boxes.x = location_x1;
		newobj.gt_boxes.y = location_y1;
		newobj.gt_boxes.width = location_x2 - location_x1;
		newobj.gt_boxes.height = location_y2 - location_y1;

		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point;
			point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x + trans_x;
			point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y + trans_y;
		
			// 修正shape点
			point.x = point.x < 0 ? 0 : point.x;
			point.y = point.y < 0 ? 0 : point.y;
			point.x = point.x > input_img.cols ? input_img.cols - 1 : point.x;
			point.y = point.y > input_img.rows ? input_img.rows - 1 : point.y;

			newobj.gt_shapes.push_back(point);
		}
		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Zoom(cv::Mat input_img, xmlReadWrite input_xml, float zoom_factor_min, float zoom_factor_max)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u1(zoom_factor_min*10, zoom_factor_max*10);
	uniform_int_distribution<unsigned> u2(0, 9);

	float zoom_factor_value = u1(e1)/10.0;
	while (zoom_factor_value == 1.0)
	{
		zoom_factor_value = u1(e1) / 10.0;
	}
	zoom_factor_value += u2(e1) / 100.0;

	int im_w = int(input_img.cols*zoom_factor_value);
	int im_h = int(input_img.rows*zoom_factor_value);

	cv::resize(input_img, output_img, cv::Size(im_w, im_h), 0, 0, cv::INTER_AREA);

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
		newobj.gt_boxes.x = input_xml.xml_object_vec[idx_obj].gt_boxes.x*zoom_factor_value;
		newobj.gt_boxes.y = input_xml.xml_object_vec[idx_obj].gt_boxes.y*zoom_factor_value;
		newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width*zoom_factor_value;
		newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height*zoom_factor_value;
		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point;
			point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x*zoom_factor_value;
			point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y*zoom_factor_value;
			newobj.gt_shapes.push_back(point);
		}
		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Resize(cv::Mat input_img, xmlReadWrite input_xml, float factor_min_x, float factor_max_x, float factor_min_y, float factor_max_y)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u1(factor_min_x*10, factor_max_x*10);
	uniform_int_distribution<unsigned> u2(factor_min_y*10, factor_max_y*10);
	uniform_int_distribution<unsigned> u3(0, 9);
	uniform_int_distribution<unsigned> u4(0, 9);

	float factor_value_x = u1(e1)/10.0 + u3(e1)/100.0;
	float factor_value_y = u2(e1)/10.0 + u4(e1)/100.0;

	int im_w = int(input_img.cols*factor_value_x);
	int im_h = int(input_img.rows*factor_value_y);

	cv::resize(input_img, output_img, cv::Size(im_w, im_h), 0, 0, cv::INTER_AREA);

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
		newobj.gt_boxes.x = input_xml.xml_object_vec[idx_obj].gt_boxes.x*factor_value_x;
		newobj.gt_boxes.y = input_xml.xml_object_vec[idx_obj].gt_boxes.y*factor_value_y;
		newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width*factor_value_x;
		newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height*factor_value_y;
		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point;
			point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x*factor_value_x;
			point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y*factor_value_y;
			newobj.gt_shapes.push_back(point);
		}
		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Shearing(cv::Mat input_img, xmlReadWrite input_xml, float shear_x, float shear_y)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u1(0, shear_x * 100);
	uniform_int_distribution<unsigned> u2(0, shear_y * 100);
	uniform_int_distribution<unsigned> u3(0,9);
	uniform_int_distribution<unsigned> u4(0,9);

	float flag_x = u3(e1) > 5 ? 1 : -1;
	float flag_y = u4(e1) > 5 ? 1 : -1;

	float shearing_x = flag_x * u1(e1) / 100.0;
	float shearing_y = flag_y * u2(e1) / 100.0;

	// https://stackoverflow.com/questions/46998895/image-shearing-c
	while (shearing_x*shearing_y == 1.0)
	{
		shearing_x = flag_x * u1(e1) / 100.0;
		shearing_y = flag_y * u2(e1) / 100.0;
	}

	if (input_img.type() != CV_8UC3)
	{
		throw("Shearing: image type is not CV_8UC3!");
	}

	// shearing:  x'=x+y·Bx   y'=y+x*By

	// shear the extreme positions to find out new image size:
	std::vector<cv::Point2f> extremePoints;
	extremePoints.push_back(cv::Point2f(0, 0));
	extremePoints.push_back(cv::Point2f(input_img.cols, 0));
	extremePoints.push_back(cv::Point2f(input_img.cols, input_img.rows));
	extremePoints.push_back(cv::Point2f(0, input_img.rows));

	for (unsigned int i = 0; i < extremePoints.size(); ++i)
	{
		cv::Point2f & pt = extremePoints[i];
		pt = cv::Point2f(pt.x + pt.y*shearing_x, pt.y + pt.x*shearing_y);
	}

	cv::Rect offsets = cv::boundingRect(extremePoints);

	cv::Point2f offset = -offsets.tl();
	cv::Size resultSize = offsets.size();

	// every pixel here is implicitely shifted by "offset"
	cv::Mat shearedImage = cv::Mat::zeros(resultSize, input_img.type()); 

	// perform the shearing by back-transformation																	 
	for (int j = 0; j < shearedImage.rows; ++j)
	{
		for (int i = 0; i < shearedImage.cols; ++i)
		{
			cv::Point2f pp(i, j);
			// go back to original coordinate system
			pp = pp - offset; 

			// go back to original pixel:
			// x'= x + y * Bx
			// y'= y + x * By
			// y = y'- x * By
			// x = x' -(y'-x * By) * Bx 
			// x = + x * By * Bx - y' * Bx + x'
			// x * (1 - By * Bx) = -y'* Bx + x'
			// x = (-y' * Bx +x')/(1 - By * Bx)

			cv::Point2f p;
			p.x = (-pp.y*shearing_x + pp.x) / (1 - shearing_y*shearing_x);
			p.y = pp.y - p.x*shearing_y;

			if ((p.x >= 0 && p.x < input_img.cols - 1) && (p.y >= 0 && p.y < input_img.rows - 1))
			{
				// TODO: interpolate, if wanted (p is floating point precision and can be placed between two pixels)!
				shearedImage.at<cv::Vec3b>(j, i) = input_img.at<cv::Vec3b>(p);
			}
		}
	}

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;

		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			float x_old = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x + offset.x;
			float y_old = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y + offset.y;
			float x_new = x_old + y_old * shearing_x;
			float y_new = y_old + x_old * shearing_y;

			cv::Point point;
			point.x = x_new;
			point.y = y_new;
			newobj.gt_shapes.push_back(point);
		}

		// 外接正矩形
		cv::Rect shape_boundary = cv::boundingRect(newobj.gt_shapes);
		newobj.gt_boxes = shape_boundary;

		output_xml.xml_object_vec.push_back(newobj);
	}

	output_img = shearedImage.clone();
	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite>  img_Crop(cv::Mat input_img, xmlReadWrite input_xml)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	// 获取shape边界
	std::vector<int> shape_boundary = get_Min_Max_Boundary(input_xml);
	int min_x = shape_boundary[0];
	int min_y = shape_boundary[1];
	int max_x = shape_boundary[2];
	int max_y = shape_boundary[3];

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(0, min_x);
	uniform_int_distribution<unsigned> u1(0, min_y);
	uniform_int_distribution<unsigned> u2(max_x, input_img.cols);
	uniform_int_distribution<unsigned> u3(max_y, input_img.rows);

	int cut_min_x = u0(e1);
	int cut_min_y = u1(e1);
	int cut_max_x = u2(e1);
	int cut_max_y = u3(e1);

	output_img = input_img(cv::Range(cut_min_y, cut_max_y), cv::Range(cut_min_x, cut_max_x));

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
		newobj.gt_boxes.x = input_xml.xml_object_vec[idx_obj].gt_boxes.x - cut_min_x;
		newobj.gt_boxes.y = input_xml.xml_object_vec[idx_obj].gt_boxes.y - cut_min_y;
		newobj.gt_boxes.width = input_xml.xml_object_vec[idx_obj].gt_boxes.width;
		newobj.gt_boxes.height = input_xml.xml_object_vec[idx_obj].gt_boxes.height;
		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Point point;
			point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x - cut_min_x;
			point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y - cut_min_y;
			newobj.gt_shapes.push_back(point);
		}
		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Affine(cv::Mat input_img, xmlReadWrite input_xml, float affine_x, float affine_y)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());

	int Affine_xa = affine_x * input_img.cols*(-1);
	int Affine_xb = affine_x * input_img.cols;

	int Affine_ya = affine_y * input_img.rows*(-1);
	int Affine_yb = affine_y * input_img.rows;

	uniform_int_distribution<unsigned> u0(0, Affine_xb - Affine_xa + 1);
	uniform_int_distribution<unsigned> u1(0, Affine_yb - Affine_ya + 1);


	int Affine_value_x1 = u0(e1) + Affine_xa;
	int Affine_value_x2 = u0(e1) + Affine_xa;
	int Affine_value_x3 = u0(e1) + Affine_xa;
	int Affine_value_x4 = u0(e1) + Affine_xa;


	int Affine_value_y1 = u1(e1) + Affine_ya;
	int Affine_value_y2 = u1(e1) + Affine_ya;
	int Affine_value_y3 = u1(e1) + Affine_ya;
	int Affine_value_y4 = u1(e1) + Affine_ya;

	cv::Point2f AffinePoints0[4] = { cv::Point2f(0, 0), cv::Point2f(0, input_img.rows), cv::Point2f(input_img.cols, 0), cv::Point2f(input_img.cols, input_img.rows) };
	cv::Point2f AffinePoints1[4] = { cv::Point2f(Affine_value_x1, Affine_value_y1), cv::Point2f(Affine_value_x2, input_img.rows - Affine_value_y2), cv::Point2f(input_img.cols - Affine_value_x3, Affine_value_y3), cv::Point2f(input_img.cols - Affine_value_x4, input_img.rows - Affine_value_y4) };

	cv::Mat Trans = cv::getAffineTransform(AffinePoints0, AffinePoints1);
	warpAffine(input_img, output_img, Trans, cv::Size(input_img.cols, input_img.rows), CV_INTER_CUBIC);

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;

		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Mat A(3, 1, CV_64FC1);
			A.at<double>(0, 0) = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
			A.at<double>(1, 0) = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
			A.at<double>(2, 0) = 1.0;

			Mat x = Trans * A;

			cv::Point point;
			point.x = int(x.at<double>(0, 0));
			point.y = int(x.at<double>(1, 0));
			newobj.gt_shapes.push_back(point);
		}
		// 外接正矩形
		cv::Rect shape_boundary = cv::boundingRect(newobj.gt_shapes);
		newobj.gt_boxes = shape_boundary;

		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::pair<cv::Mat, xmlReadWrite> img_Rotate(cv::Mat input_img, xmlReadWrite input_xml, int rotate_angle_start, int rotate_angle_end)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(0, 50);
	uniform_int_distribution<unsigned> u1(rotate_angle_start, rotate_angle_end);
	uniform_int_distribution<unsigned> u2(0, 9);

	int center_x = 0.5*input_img.cols;
	int center_y = 0.5*input_img.rows;

	cv::Point2f rotate_center(center_x + u0(e1), center_y + u0(e1));

	int flag_rotate = u2(e1) >= 5 ? 1 : -1;

	int rotate_angle = u1(e1)*flag_rotate;

	cv::Mat rotMat = cv::getRotationMatrix2D(rotate_center, rotate_angle, 1);
	cv::warpAffine(input_img, output_img, rotMat, input_img.size());

	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = input_xml.img_width;
	output_xml.img_hight = input_xml.img_hight;

	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;

		for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
			cv::Mat A(3, 1, CV_64FC1);
			A.at<double>(0, 0) = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x;
			A.at<double>(1, 0) = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y;
			A.at<double>(2, 0) = 1.0;

			Mat x = rotMat * A;

			cv::Point point;
			point.x = int(x.at<double>(0, 0));
			point.y = int(x.at<double>(1, 0));
			newobj.gt_shapes.push_back(point);
		}
		// 外接正矩形
		cv::Rect shape_boundary = cv::boundingRect(newobj.gt_shapes);
		newobj.gt_boxes = shape_boundary;

		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;

	return output;
}

cv::Mat img_Perspective(cv::Mat src, cv::Point2f* scrPoints, cv::Point2f* dstPoints)
{
	cv::Mat dst;
	cv::Mat Trans = cv::getPerspectiveTransform(scrPoints, dstPoints);
	warpPerspective(src, dst, Trans, cv::Size(src.cols, src.rows), CV_INTER_CUBIC);
	return dst;
}

void boxAug(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<cv::Mat> &img_rois, bool flag_img_show)
{
	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<cv::Rect> boxes_generate;

	int num_rotate_angle = int(360 / aug_parameter.scale_angle);
	int num_box_in_an_angle = int(aug_parameter.num_aug_max / num_rotate_angle);
	num_box_in_an_angle = num_box_in_an_angle > 1 ? num_box_in_an_angle : 1;

	for (int idx_obj = 0; idx_obj < annotation.xml_object_vec.size(); idx_obj++)
	{
		cv::Rect obj_box = annotation.xml_object_vec[idx_obj].gt_boxes;
		std::vector<cv::Point> box_shapes = annotation.xml_object_vec[idx_obj].gt_shapes;

		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u_scale(aug_parameter.sacle_min * 100, aug_parameter.scale_max * 100);
		uniform_int_distribution<signed> u_angle(-int(0.5*aug_parameter.scale_angle), int(0.5*aug_parameter.scale_angle));
		uniform_int_distribution<unsigned> u_center(aug_parameter.scale_min_r * 100, aug_parameter.scale_max_r * 100);

		for (int idx_angle = 0; idx_angle < num_rotate_angle; idx_angle++)
		{
			int num_box_generate_in_this_angle = 0;
			int num_count = 0;
			do
			{
				num_count++;
				float img_scale_x = u_scale(e1) / 100.0;
				float img_scale_y = u_scale(e1) / 100.0;
				float value_angle = aug_parameter.scale_angle * idx_angle + u_angle(e1);

				int radius_w = u_center(e1) / 100.0 * obj_box.width;
				int radius_h = u_center(e1) / 100.0 * obj_box.height;

				int radius_elips = 1.0*radius_w* radius_h*std::sqrt(1.0 / ((radius_w*radius_w*std::pow(std::cos(value_angle), 2)) + (radius_h*radius_h*std::pow(std::sin(value_angle), 2))));
				uniform_int_distribution<unsigned> u_radius(0, radius_elips);;
				int random_radius = u_radius(e1);

				int x = obj_box.x + obj_box.width *0.5 + random_radius * std::cos(value_angle);
				int y = obj_box.y + obj_box.height*0.5 + random_radius * std::sin(value_angle);
				int bias_w = obj_box.width  * img_scale_x;
				int bias_h = obj_box.height * img_scale_y;

				cv::Rect new_box = cv::Rect(x - 0.5*bias_w, y - 0.5*bias_h, bias_w, bias_h);
				new_box.x = 0 > new_box.x ? 0 : new_box.x;
				new_box.y = 0 > new_box.y ? 0 : new_box.y;
				new_box.x = img.cols < new_box.x ? img.cols : new_box.x;
				new_box.y = img.rows < new_box.y ? img.rows : new_box.y;
				new_box.width = img.cols < (new_box.x + new_box.width) ? img.cols - new_box.x : new_box.width;
				new_box.height = img.rows < (new_box.y + new_box.height) ? img.rows - new_box.y : new_box.height;

				float old_w_divide_h = 1.0*obj_box.width / obj_box.height;
				float old_h_divide_w = 1.0 / old_w_divide_h;
				float new_w_divide_h = 1.0*new_box.width / new_box.height;
				float new_h_divide_w = 1.0 / new_w_divide_h;
				float max_ratio_old = old_w_divide_h > old_h_divide_w ? old_w_divide_h : old_h_divide_w;
				float max_ratio_new = new_w_divide_h > new_h_divide_w ? new_w_divide_h : new_h_divide_w;

				if (max_ratio_new > aug_parameter.ratio_max * max_ratio_old)
				{
					continue;
				}

				if (flag_img_show)
				{
					cv::Point point;
					point.x = x;
					point.y = y;
					circle(img, point, 3, Scalar(0, 255, 255), -1, 8, 0);
				}

				float iou = 0.0;
				if (aug_parameter.flag_overlap)
				{
					std::vector<std::vector<cv::Point>> annotation_shapes;
					std::vector<cv::Point> box_shapes_refine = box_shapes;
					for (int idx_point = 0; idx_point < box_shapes.size(); idx_point++)
					{
						box_shapes_refine[idx_point].x = box_shapes_refine[idx_point].x - new_box.x;
						box_shapes_refine[idx_point].y = box_shapes_refine[idx_point].y - new_box.y;
					}
					annotation_shapes.push_back(box_shapes_refine);
					cv::Mat img_mark = cv::Mat(cv::Size(new_box.width, new_box.height), CV_8UC1, cv::Scalar(0));
					cv::drawContours(img_mark, annotation_shapes, -1, cv::Scalar(1), -1);

					cv::Scalar roi_generate_area = sum(img_mark);

					iou = 1.0*roi_generate_area[0] / (new_box.width*new_box.height);
				}
				else
				{
					iou = 1.0*(obj_box & new_box).area() / (new_box.area() + obj_box.area() - (obj_box & new_box).area());
				}
				if (iou>aug_parameter.value_iou)
				{
					num_box_generate_in_this_angle++;
					if ((aug_parameter.height_pad) && (aug_parameter.width_pad))
					{
						new_box.x = 0 >new_box.x - aug_parameter.width_pad ? 0 : new_box.x - aug_parameter.width_pad;
						new_box.y = 0 > new_box.y - aug_parameter.height_pad ? 0 : new_box.y - aug_parameter.height_pad;
						new_box.width = img.cols < (new_box.x + new_box.width + 2 * aug_parameter.width_pad) ? img.cols - new_box.x : (new_box.width + 2 * aug_parameter.width_pad);
						new_box.height = img.rows < (new_box.y + new_box.height + 2 * aug_parameter.height_pad) ? img.rows - new_box.y : (new_box.height + 2 * aug_parameter.height_pad);
					}
					boxes_generate.push_back(new_box);
					img_rois.push_back(img(new_box));
				}
			} while ((num_box_generate_in_this_angle<num_box_in_an_angle) && (num_count<200));
		}
		//if ((aug_parameter.height_pad) && (aug_parameter.width_pad))
		//{
			obj_box.x = 0 >obj_box.x - aug_parameter.width_pad ? 0 : obj_box.x - aug_parameter.width_pad;
			obj_box.y = 0 > obj_box.y - aug_parameter.height_pad ? 0 : obj_box.y - aug_parameter.height_pad;
			obj_box.width = img.cols < (obj_box.x + obj_box.width + 2 * aug_parameter.width_pad) ? img.cols - obj_box.x : (obj_box.width + 2 * aug_parameter.width_pad);
			obj_box.height = img.rows < (obj_box.y + obj_box.height + 2 * aug_parameter.height_pad) ? img.rows - obj_box.y : (obj_box.height + 2 * aug_parameter.height_pad);
		//}
		boxes_generate.push_back(obj_box);
		img_rois.push_back(img(obj_box));
	}
	if (flag_img_show)
	{
		for (int idx = 0; idx < annotation.xml_object_vec.size(); idx++)
		{
			cv::rectangle(img, annotation.xml_object_vec[idx].gt_boxes, cv::Scalar(0, 0, 255), 2);

		}
		for (int idx = 0; idx < boxes_generate.size(); idx++)
		{
			cv::rectangle(img, boxes_generate[idx], cv::Scalar(idx * 1 % 255, idx * 5 % 255, idx * 10 % 255), 1);
		}
		cv::resize(img, img, cv::Size(img.cols * 0.5, img.rows * 0.5), 0, 0, cv::INTER_AREA);
		imshow("img_show", img);
		waitKey(0);
	}
}

void boxAug2(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<cv::Mat> &img_rois, std::vector<int> &label_rois, bool flag_img_show)
{
	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<cv::Rect> boxes_generate;

	// 旋转数目，以及每个角度上生成框的数目
	int num_rotate_angle = int(360 / aug_parameter.scale_angle);
	int num_box_in_an_angle = int(aug_parameter.num_aug_max / num_rotate_angle);
	num_box_in_an_angle = num_box_in_an_angle > 1 ? num_box_in_an_angle : 1;

	for (int idx_obj = 0; idx_obj < annotation.xml_object_vec.size(); idx_obj++)
	{
		// new add here by lilai 2019.02.15
		std::vector<cv::Rect> idx_boxes_generate;

		cv::Rect obj_box = annotation.xml_object_vec[idx_obj].gt_boxes;
		std::vector<cv::Point> box_shapes = annotation.xml_object_vec[idx_obj].gt_shapes;

		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u_scale(aug_parameter.sacle_min * 100, aug_parameter.scale_max * 100);
		uniform_int_distribution<signed> u_angle(-int(0.5*aug_parameter.scale_angle), int(0.5*aug_parameter.scale_angle));
		uniform_int_distribution<unsigned> u_center(aug_parameter.scale_min_r * 100, aug_parameter.scale_max_r * 100);

		// new add here by lilai 2019.02.15
		std::vector<std::vector<cv::Point>> img_ori_shape;
		img_ori_shape.push_back(box_shapes);
		cv::Mat img_ori_mark = cv::Mat(img.size(), CV_8UC1, cv::Scalar(0));
		cv::drawContours(img_ori_mark, img_ori_shape, -1, cv::Scalar(255), -1);

		for (int idx_angle = 0; idx_angle < num_rotate_angle; idx_angle++)
		{
			int num_box_generate_in_this_angle = 0;
			int num_count = 0;
			do
			{			
				float img_scale_x = u_scale(e1) / 100.0;
				float img_scale_y = u_scale(e1) / 100.0;
				float value_angle = aug_parameter.scale_angle * idx_angle + u_angle(e1);

				int radius_w = u_center(e1) / 100.0 * obj_box.width;
				int radius_h = u_center(e1) / 100.0 * obj_box.height;

				int radius_elips = 1.0*radius_w* radius_h*std::sqrt(1.0 / ((radius_w*radius_w*std::pow(std::cos(value_angle), 2)) + (radius_h*radius_h*std::pow(std::sin(value_angle), 2))));
				uniform_int_distribution<unsigned> u_radius(0, radius_elips);;
				int random_radius = u_radius(e1);

				int x = obj_box.x + obj_box.width *0.5 + random_radius * std::cos(value_angle);
				int y = obj_box.y + obj_box.height*0.5 + random_radius * std::sin(value_angle);
				int bias_w = obj_box.width  * img_scale_x;
				int bias_h = obj_box.height * img_scale_y;

				cv::Rect new_box = cv::Rect(x - 0.5*bias_w, y - 0.5*bias_h, bias_w, bias_h);
				new_box.x = 0 > new_box.x ? 0 : new_box.x;
				new_box.y = 0 > new_box.y ? 0 : new_box.y;
				new_box.x = img.cols - 1 < new_box.x ? img.cols - 1 : new_box.x;
				new_box.y = img.rows - 1 < new_box.y ? img.rows - 1 : new_box.y;
				new_box.width = img.cols - 1 < (new_box.x + new_box.width) ? img.cols - 1 - new_box.x : new_box.width;
				new_box.height = img.rows - 1 < (new_box.y + new_box.height) ? img.rows - 1 - new_box.y : new_box.height;

				float old_w_divide_h = 1.0*obj_box.width / obj_box.height;
				float old_h_divide_w = 1.0 / old_w_divide_h;
				float new_w_divide_h = 1.0*new_box.width / new_box.height;
				float new_h_divide_w = 1.0 / new_w_divide_h;
				float max_ratio_old = old_w_divide_h > old_h_divide_w ? old_w_divide_h : old_h_divide_w;
				float max_ratio_new = new_w_divide_h > new_h_divide_w ? new_w_divide_h : new_h_divide_w;

				// 剔除异常长宽比
				if (max_ratio_new > aug_parameter.ratio_max * max_ratio_old)
				{
					continue;
				}
				// 剔除单边异常放大
				if (new_box.width > obj_box.width * aug_parameter.value_zoom)
				{
					continue;
				}
				if (new_box.height > obj_box.height * aug_parameter.value_zoom)
				{
					continue;
				}

				num_count++;

				if (flag_img_show)
				{
					cv::Point point;
					point.x = x;
					point.y = y;
					circle(img, point, 3, Scalar(0, 255, 255), -1, 8, 0);
				}

				// box-iou:交集/并集；shape-iou：交集/生成框
				float iou = 0.0;
				if (aug_parameter.flag_overlap)
				{
					std::vector<std::vector<cv::Point>> annotation_shapes;
					std::vector<cv::Point> box_shapes_refine = box_shapes;
					for (int idx_point = 0; idx_point < box_shapes.size(); idx_point++)
					{
						box_shapes_refine[idx_point].x = box_shapes_refine[idx_point].x - new_box.x;
						box_shapes_refine[idx_point].y = box_shapes_refine[idx_point].y - new_box.y;
					}
					annotation_shapes.push_back(box_shapes_refine);
					cv::Mat img_mark = cv::Mat(cv::Size(new_box.width, new_box.height), CV_8UC1, cv::Scalar(0));
					cv::drawContours(img_mark, annotation_shapes, -1, cv::Scalar(1), -1);

					cv::Scalar roi_generate_area = sum(img_mark);

					iou = 1.0*roi_generate_area[0] / (new_box.width*new_box.height);
				}
				else
				{
					iou = 1.0*(obj_box & new_box).area() / (new_box.area() + obj_box.area() - (obj_box & new_box).area());
				}
				if (iou>aug_parameter.value_iou)
				{
					num_box_generate_in_this_angle++;
					if ((aug_parameter.height_pad) && (aug_parameter.width_pad))
					{
						new_box.x = 0 > new_box.x - aug_parameter.width_pad ? 0 : new_box.x - aug_parameter.width_pad;
						new_box.y = 0 > new_box.y - aug_parameter.height_pad ? 0 : new_box.y - aug_parameter.height_pad;
						new_box.width = img.cols - 1 < (new_box.x + new_box.width + 2*aug_parameter.width_pad) ? img.cols - 1 - new_box.x : (new_box.width + 2*aug_parameter.width_pad);
						new_box.height = img.rows - 1 < (new_box.y + new_box.height + 2*aug_parameter.height_pad) ? img.rows - 1 - new_box.y : (new_box.height + 2*aug_parameter.height_pad);
					}
					if (new_box.width>0 && new_box.height>0)
					{
						idx_boxes_generate.push_back(new_box);
					}
				}
			} while ((num_box_generate_in_this_angle<num_box_in_an_angle) && (num_count<200));
		}

		obj_box.x = 0 > obj_box.x - aug_parameter.width_pad ? 0 : obj_box.x - aug_parameter.width_pad;
		obj_box.y = 0 > obj_box.y - aug_parameter.height_pad ? 0 : obj_box.y - aug_parameter.height_pad;
		obj_box.width = img.cols - 1 < (obj_box.x + obj_box.width + 2 * aug_parameter.width_pad) ? img.cols - 1 - obj_box.x : (obj_box.width + 2 * aug_parameter.width_pad);
		obj_box.height = img.rows - 1 < (obj_box.y + obj_box.height + 2 * aug_parameter.height_pad) ? img.rows - 1 - obj_box.y : (obj_box.height + 2 * aug_parameter.height_pad);
		
		if (obj_box.width>0 && obj_box.height>0)
		{
			idx_boxes_generate.push_back(obj_box);
		}

		// new add here by lilai 2019.02.15【Tips:可能会出现较大方框，可以将下面的代码注释】
		for (int idx = 0; idx < idx_boxes_generate.size(); idx++)
		{
			cv::Rect box_check = idx_boxes_generate[idx];
			std::vector<std::vector<cv::Point>> idx_contours;

			cv::Mat roi_on_mark = img_ori_mark(box_check);
			cv::findContours(roi_on_mark.clone(), idx_contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
			std::vector<Moments> mu(idx_contours.size());

			/*
			// stage1：对齐：将目标中心移动至生成框的中心
			if (idx_contours.size()<=0)
			{
				continue;
			}
			// 中心距
			for (int i = 0; i < idx_contours.size(); i++)
			{
				mu[i] = moments(idx_contours[i], false);
			}

			// 中心点
			std::vector<Point> mc(idx_contours.size());

			for (int i = 0; i < idx_contours.size(); i++)
			{
				mc[i] = Point(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			}

			// 质心
			cv::Point point_center;
			for (int i = 0; i < idx_contours.size(); i++)
			{		
				point_center.x = point_center.x + mc[i].x + box_check.x;
				point_center.y = point_center.y + mc[i].y + box_check.y;
			}
			point_center.x = point_center.x / idx_contours.size();
			point_center.y = point_center.y / idx_contours.size();
			
			// 距离
			int move_distance_x = abs(point_center.x - (box_check.x + box_check.width)) ;
			int move_distance_y = abs(point_center.y - (box_check.y + box_check.height));
			
			// 移动
			if (move_distance_x > aug_parameter.value_move_x * box_check.width)
			{
				box_check.x = point_center.x - 0.5*box_check.width;
			}
			if (move_distance_y > aug_parameter.value_move_y * box_check.height)
			{
				box_check.y = point_center.y - 0.5*box_check.height;
			}

			// 越界
			box_check.x = 0 > box_check.x ? 0 : box_check.x;
			box_check.y = 0 > box_check.y ? 0 : box_check.y;
			box_check.x = img.cols - 1 < box_check.x ? img.cols - 1 : box_check.x;
			box_check.y = img.rows - 1 < box_check.y ? img.rows - 1 : box_check.y;
			box_check.width = img.cols - 1 < (box_check.x + box_check.width) ? img.cols - 1 - box_check.x : box_check.width;
			box_check.height = img.rows - 1 < (box_check.y + box_check.height) ? img.rows - 1 - box_check.y : box_check.height;

			// stage2：修正：将短边pad成长边的尺寸
			float value_x_divide_y = 1.0 * box_check.width / box_check.height;
			float value_y_divide_x = 1.0 * box_check.height / box_check.width;
			float value_divide_ratio = value_x_divide_y > value_y_divide_x ? value_x_divide_y : value_y_divide_x;
			if (value_divide_ratio>aug_parameter.value_ratio)
			{
				if (value_x_divide_y>1.0)
				{
					int pad_h = (box_check.width - box_check.height) / 2;
					box_check.y -= pad_h;
					box_check.height = box_check.width;
				}
				else
				{
					int pad_w = (box_check.height - box_check.width) / 2;
					box_check.x -= pad_w;
					box_check.width = box_check.height;
				}

				// 越界
				box_check.x = 0 > box_check.x ? 0 : box_check.x;
				box_check.y = 0 > box_check.y ? 0 : box_check.y;
				box_check.x = img.cols - 1 < box_check.x ? img.cols - 1 : box_check.x;
				box_check.y = img.rows - 1  < box_check.y ? img.rows - 1 : box_check.y;
				box_check.width = img.cols - 1 < (box_check.x + box_check.width) ? img.cols - 1 - box_check.x : box_check.width;
				box_check.height = img.rows - 1 < (box_check.y + box_check.height) ? img.rows - 1 - box_check.y : box_check.height;
			}

			*/

			boxes_generate.push_back(box_check);
			img_rois.push_back(img(box_check));
			label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);

			// 验证
			//cv::Mat img_show = img_ori_mark.clone();
			//cv::circle(img_show, point_center, 20, cv::Scalar(125), -1, 8, 0);
			//cv::rectangle(img_show, idx_boxes_generate[idx], cv::Scalar(255), 15);
			//cv::rectangle(img_show, box_check, cv::Scalar(125), 15);
			//cv::resize(img_show, img_show,cv::Size(0.25*img_show.cols, 0.25*img_show.rows));
			//cv::imshow("img_show", img_show);
			//cv::waitKey(0);
		}
	}
	if (flag_img_show)
	{
		for (int idx = 0; idx < annotation.xml_object_vec.size(); idx++)
		{
			cv::rectangle(img, annotation.xml_object_vec[idx].gt_boxes, cv::Scalar(0, 0, 255), 2);

		}
		for (int idx = 0; idx < boxes_generate.size(); idx++)
		{
			cv::rectangle(img, boxes_generate[idx], cv::Scalar(idx * 1 % 255, idx * 5 % 255, idx * 10 % 255), 1);
		}
		cv::resize(img, img, cv::Size(img.cols * 0.5, img.rows * 0.5), 0, 0, cv::INTER_AREA);
		imshow("img_show", img);
		waitKey(0);
	}
}

void rotate_arbitrarily_angle(Mat &src, Mat &dst, float angle)
{
	float radian = (float)(angle / 180.0 * CV_PI);

	//填充图像
	int maxBorder = (int)(max(src.cols, src.rows)* 1.414); //即为sqrt(2)*max
	int dx = (maxBorder - src.cols) / 2;
	int dy = (maxBorder - src.rows) / 2;
	copyMakeBorder(src, dst, dy, dy, dx, dx, BORDER_CONSTANT);

	//旋转
	Point2f center((float)(dst.cols / 2), (float)(dst.rows / 2));
	Mat affine_matrix = getRotationMatrix2D(center, angle, 1.0);//求得旋转矩阵
	warpAffine(dst, dst, affine_matrix, dst.size());

	//计算图像旋转之后包含图像的最大的矩形
	float sinVal = abs(sin(radian));
	float cosVal = abs(cos(radian));
	Size targetSize((int)(src.cols * cosVal + src.rows * sinVal),
		(int)(src.cols * sinVal + src.rows * cosVal));

	//剪掉多余边框
	int x = (dst.cols - targetSize.width) / 2;
	int y = (dst.rows - targetSize.height) / 2;
	Rect rect(x, y, targetSize.width, targetSize.height);
	dst = Mat(dst, rect);
}

void boxAug2_longjump(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<cv::Mat> &img_rois, std::vector<int> &label_rois, bool flag_img_show, std::vector<cv::Point> innerpoints, std::vector<cv::Point> outerpoints)
{
	//先绕-->再扩-->统一朝向-->统一宽高比-


	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<cv::Rect> boxes_generate;

	// 旋转数目，以及每个角度上生成框的数目
	int num_rotate_angle = int(360 / aug_parameter.scale_angle);
	int num_box_in_an_angle = int(aug_parameter.num_aug_max / num_rotate_angle);
	num_box_in_an_angle = num_box_in_an_angle > 1 ? num_box_in_an_angle : 1;

	for (int idx_obj = 0; idx_obj < annotation.xml_object_vec.size(); idx_obj++)
	{
		// new add here by lilai 2019.02.15
		std::vector<cv::Rect> idx_boxes_generate;

		cv::Rect obj_box = annotation.xml_object_vec[idx_obj].gt_boxes;
		std::vector<cv::Point> box_shapes = annotation.xml_object_vec[idx_obj].gt_shapes;

		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u_scale(aug_parameter.sacle_min * 100, aug_parameter.scale_max * 100);
		uniform_int_distribution<signed> u_angle(-int(0.5*aug_parameter.scale_angle), int(0.5*aug_parameter.scale_angle));
		uniform_int_distribution<unsigned> u_center(aug_parameter.scale_min_r * 100, aug_parameter.scale_max_r * 100);

		// new add here by lilai 2019.02.15
		std::vector<std::vector<cv::Point>> img_ori_shape;
		img_ori_shape.push_back(box_shapes);
		cv::Mat img_ori_mark = cv::Mat(img.size(), CV_8UC1, cv::Scalar(0));
		cv::drawContours(img_ori_mark, img_ori_shape, -1, cv::Scalar(255), -1);

		for (int idx_angle = 0; idx_angle < num_rotate_angle; idx_angle++)
		{
			int num_box_generate_in_this_angle = 0;
			int num_count = 0;
			do
			{
				float img_scale_x = u_scale(e1) / 100.0;
				float img_scale_y = u_scale(e1) / 100.0;
				float value_angle = aug_parameter.scale_angle * idx_angle + u_angle(e1);

				int radius_w = u_center(e1) / 100.0 * obj_box.width;
				int radius_h = u_center(e1) / 100.0 * obj_box.height;

				int radius_elips = 1.0*radius_w* radius_h*std::sqrt(1.0 / ((radius_w*radius_w*std::pow(std::cos(value_angle), 2)) + (radius_h*radius_h*std::pow(std::sin(value_angle), 2))));
				uniform_int_distribution<unsigned> u_radius(0, radius_elips);;
				int random_radius = u_radius(e1);

				int x = obj_box.x + obj_box.width *0.5 + random_radius * std::cos(value_angle);
				int y = obj_box.y + obj_box.height*0.5 + random_radius * std::sin(value_angle);
				int bias_w = obj_box.width  * img_scale_x;
				int bias_h = obj_box.height * img_scale_y;

				cv::Rect new_box = cv::Rect(x - 0.5*bias_w, y - 0.5*bias_h, bias_w, bias_h);
				new_box.x = 0 > new_box.x ? 0 : new_box.x;
				new_box.y = 0 > new_box.y ? 0 : new_box.y;
				new_box.x = img.cols - 1 < new_box.x ? img.cols - 1 : new_box.x;
				new_box.y = img.rows - 1 < new_box.y ? img.rows - 1 : new_box.y;
				new_box.width = img.cols - 1 < (new_box.x + new_box.width) ? img.cols - 1 - new_box.x : new_box.width;
				new_box.height = img.rows - 1 < (new_box.y + new_box.height) ? img.rows - 1 - new_box.y : new_box.height;

				float old_w_divide_h = 1.0*obj_box.width / obj_box.height;
				float old_h_divide_w = 1.0 / old_w_divide_h;
				float new_w_divide_h = 1.0*new_box.width / new_box.height;
				float new_h_divide_w = 1.0 / new_w_divide_h;
				float max_ratio_old = old_w_divide_h > old_h_divide_w ? old_w_divide_h : old_h_divide_w;
				float max_ratio_new = new_w_divide_h > new_h_divide_w ? new_w_divide_h : new_h_divide_w;

				// 剔除异常长宽比
				if (max_ratio_new > aug_parameter.ratio_max * max_ratio_old)
				{
					continue;
				}
				// 剔除单边异常放大
				if (new_box.width > obj_box.width * aug_parameter.value_zoom)
				{
					continue;
				}
				if (new_box.height > obj_box.height * aug_parameter.value_zoom)
				{
					continue;
				}

				num_count++;

				if (flag_img_show)
				{
					cv::Point point;
					point.x = x;
					point.y = y;
					circle(img, point, 3, Scalar(0, 255, 255), -1, 8, 0);
				}

				// box-iou:交集/并集；shape-iou：交集/生成框
				float iou = 0.0;
				if (aug_parameter.flag_overlap)
				{
					std::vector<std::vector<cv::Point>> annotation_shapes;
					std::vector<cv::Point> box_shapes_refine = box_shapes;
					for (int idx_point = 0; idx_point < box_shapes.size(); idx_point++)
					{
						box_shapes_refine[idx_point].x = box_shapes_refine[idx_point].x - new_box.x;
						box_shapes_refine[idx_point].y = box_shapes_refine[idx_point].y - new_box.y;
					}
					annotation_shapes.push_back(box_shapes_refine);
					cv::Mat img_mark = cv::Mat(cv::Size(new_box.width, new_box.height), CV_8UC1, cv::Scalar(0));
					cv::drawContours(img_mark, annotation_shapes, -1, cv::Scalar(1), -1);

					cv::Scalar roi_generate_area = sum(img_mark);

					iou = 1.0*roi_generate_area[0] / (new_box.width*new_box.height);
				}
				else
				{
					iou = 1.0*(obj_box & new_box).area() / (new_box.area() + obj_box.area() - (obj_box & new_box).area());
				}
				if (iou>aug_parameter.value_iou)
				{
					num_box_generate_in_this_angle++;
					
					//扰动后的框进行扩框
					if ((aug_parameter.height_pad) && (aug_parameter.width_pad))
					{
						new_box.x = 0 > new_box.x - aug_parameter.width_pad ? 0 : new_box.x - aug_parameter.width_pad;
						new_box.y = 0 > new_box.y - aug_parameter.height_pad ? 0 : new_box.y - aug_parameter.height_pad;
						new_box.width = img.cols - 1 < (new_box.x + new_box.width + 2 * aug_parameter.width_pad) ? img.cols - 1 - new_box.x : (new_box.width + 2 * aug_parameter.width_pad);
						new_box.height = img.rows - 1 < (new_box.y + new_box.height + 2 * aug_parameter.height_pad) ? img.rows - 1 - new_box.y : (new_box.height + 2 * aug_parameter.height_pad);
					}
					if (new_box.width>0 && new_box.height>0)
					{
						idx_boxes_generate.push_back(new_box);
					}
				}
			} while ((num_box_generate_in_this_angle<num_box_in_an_angle) && (num_count<200));
		}
		
		//原始框扩框

		obj_box.x = 0 > obj_box.x - aug_parameter.width_pad ? 0 : obj_box.x - aug_parameter.width_pad;
		obj_box.y = 0 > obj_box.y - aug_parameter.height_pad ? 0 : obj_box.y - aug_parameter.height_pad;
		obj_box.width = img.cols - 1 < (obj_box.x + obj_box.width + 2 * aug_parameter.width_pad) ? img.cols - 1 - obj_box.x : (obj_box.width + 2 * aug_parameter.width_pad);
		obj_box.height = img.rows - 1 < (obj_box.y + obj_box.height + 2 * aug_parameter.height_pad) ? img.rows - 1 - obj_box.y : (obj_box.height + 2 * aug_parameter.height_pad);

		if (obj_box.width>0 && obj_box.height>0)
		{
			idx_boxes_generate.push_back(obj_box);
		}

		// new add here by lilai 2019.02.15【Tips:可能会出现较大方框，可以将下面的代码注释】
		for (int idx = 0; idx < idx_boxes_generate.size(); idx++)
		{
			cv::Rect box_check = idx_boxes_generate[idx];
			std::vector<std::vector<cv::Point>> idx_contours;

			cv::Mat roi_on_mark = img_ori_mark(box_check);
			cv::findContours(roi_on_mark.clone(), idx_contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
			std::vector<Moments> mu(idx_contours.size());

			/*
			// stage1：对齐：将目标中心移动至生成框的中心
			if (idx_contours.size()<=0)
			{
			continue;
			}
			// 中心距
			for (int i = 0; i < idx_contours.size(); i++)
			{
			mu[i] = moments(idx_contours[i], false);
			}

			// 中心点
			std::vector<Point> mc(idx_contours.size());

			for (int i = 0; i < idx_contours.size(); i++)
			{
			mc[i] = Point(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
			}

			// 质心
			cv::Point point_center;
			for (int i = 0; i < idx_contours.size(); i++)
			{
			point_center.x = point_center.x + mc[i].x + box_check.x;
			point_center.y = point_center.y + mc[i].y + box_check.y;
			}
			point_center.x = point_center.x / idx_contours.size();
			point_center.y = point_center.y / idx_contours.size();

			// 距离
			int move_distance_x = abs(point_center.x - (box_check.x + box_check.width)) ;
			int move_distance_y = abs(point_center.y - (box_check.y + box_check.height));

			// 移动
			if (move_distance_x > aug_parameter.value_move_x * box_check.width)
			{
			box_check.x = point_center.x - 0.5*box_check.width;
			}
			if (move_distance_y > aug_parameter.value_move_y * box_check.height)
			{
			box_check.y = point_center.y - 0.5*box_check.height;
			}

			// 越界
			box_check.x = 0 > box_check.x ? 0 : box_check.x;
			box_check.y = 0 > box_check.y ? 0 : box_check.y;
			box_check.x = img.cols - 1 < box_check.x ? img.cols - 1 : box_check.x;
			box_check.y = img.rows - 1 < box_check.y ? img.rows - 1 : box_check.y;
			box_check.width = img.cols - 1 < (box_check.x + box_check.width) ? img.cols - 1 - box_check.x : box_check.width;
			box_check.height = img.rows - 1 < (box_check.y + box_check.height) ? img.rows - 1 - box_check.y : box_check.height;

			// stage2：修正：将短边pad成长边的尺寸
			float value_x_divide_y = 1.0 * box_check.width / box_check.height;
			float value_y_divide_x = 1.0 * box_check.height / box_check.width;
			float value_divide_ratio = value_x_divide_y > value_y_divide_x ? value_x_divide_y : value_y_divide_x;
			if (value_divide_ratio>aug_parameter.value_ratio)
			{
			if (value_x_divide_y>1.0)
			{
			int pad_h = (box_check.width - box_check.height) / 2;
			box_check.y -= pad_h;
			box_check.height = box_check.width;
			}
			else
			{
			int pad_w = (box_check.height - box_check.width) / 2;
			box_check.x -= pad_w;
			box_check.width = box_check.height;
			}

			// 越界
			box_check.x = 0 > box_check.x ? 0 : box_check.x;
			box_check.y = 0 > box_check.y ? 0 : box_check.y;
			box_check.x = img.cols - 1 < box_check.x ? img.cols - 1 : box_check.x;
			box_check.y = img.rows - 1  < box_check.y ? img.rows - 1 : box_check.y;
			box_check.width = img.cols - 1 < (box_check.x + box_check.width) ? img.cols - 1 - box_check.x : box_check.width;
			box_check.height = img.rows - 1 < (box_check.y + box_check.height) ? img.rows - 1 - box_check.y : box_check.height;
			}


			*/



			


			//统一框中绊丝的朝向，减少复杂度,裁剪的图片进行旋转90度

			float th_innerpoint_half = (innerpoints[10].x + innerpoints[11].x)*0.5;
			int x_right = idx_boxes_generate[idx].x + idx_boxes_generate[idx].width;
			
			cv::Mat crop_img_rotate, crop_img_rotate_hwr,crop_img_hwr;
			cv::Mat crop_img = img(idx_boxes_generate[idx]);

		
			if (false)//方案1：wangwei  目前pass，由于特殊绊丝的方向不定，难以统一方向
			{
				if (x_right < th_innerpoint_half)
				{
					rotate_arbitrarily_angle(crop_img, crop_img_rotate, 270); //等同于顺时针旋转90度


																			  //统一高宽比，根据宽高比值，对框进行旋转
					float h_w_r = idx_boxes_generate[idx].height*1.0 / idx_boxes_generate[idx].width;
					if (h_w_r > 1.1)
					{
						//旋转90
						rotate_arbitrarily_angle(crop_img_rotate, crop_img_rotate_hwr, 270); //等同于顺时针旋转90度
						cv::flip(crop_img_rotate_hwr, crop_img_hwr, 0);       //关于y轴对称
						img_rois.push_back(crop_img_hwr);
						label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);
					}
					else
					{
						img_rois.push_back(crop_img_rotate);
						label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);
					}


				}
				else
				{
					img_rois.push_back(crop_img);
					label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);
				}
			}
			else 
			{
			
				//方案2：duanting

				std::vector<cv::Mat>img_hwr;


				//统一高宽比，根据宽高比值，对框进行旋转

				float h_w_r = idx_boxes_generate[idx].height*1.0 / idx_boxes_generate[idx].width;
				if (h_w_r > 1.1)
				{
					//中间的情况先不管
					if (idx_boxes_generate[idx].x > th_innerpoint_half) //右边
					{
						//旋转90
						rotate_arbitrarily_angle(crop_img, crop_img_rotate_hwr, 270); //等同于顺时针旋转90度
						img_rois.push_back(crop_img_rotate_hwr);
						label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);

					}
					else if (x_right <  th_innerpoint_half)    //左边  
					{
						//旋转90
						rotate_arbitrarily_angle(crop_img, crop_img_rotate_hwr, 90); //等同于顺时针旋转90度
						img_rois.push_back(crop_img_rotate_hwr);
						label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);

					}
					else 
					{
						img_rois.push_back(crop_img);
						label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);
					
					}
					
				}
				else
				{
					img_rois.push_back(crop_img);
					label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);
				}
			
			}
			



			boxes_generate.push_back(box_check);
			//img_rois.push_back(img(box_check));
			//label_rois.push_back(annotation.xml_object_vec[idx_obj].gt_level);

			// 验证
			//cv::Mat img_show = img_ori_mark.clone();
			//cv::circle(img_show, point_center, 20, cv::Scalar(125), -1, 8, 0);
			//cv::rectangle(img_show, idx_boxes_generate[idx], cv::Scalar(255), 15);
			//cv::rectangle(img_show, box_check, cv::Scalar(125), 15);
			//cv::resize(img_show, img_show,cv::Size(0.25*img_show.cols, 0.25*img_show.rows));
			//cv::imshow("img_show", img_show);
			//cv::waitKey(0);
		}
	}
	if (flag_img_show)
	{
		for (int idx = 0; idx < annotation.xml_object_vec.size(); idx++)
		{
			cv::rectangle(img, annotation.xml_object_vec[idx].gt_boxes, cv::Scalar(0, 0, 255), 2);

		}
		for (int idx = 0; idx < boxes_generate.size(); idx++)
		{
			cv::rectangle(img, boxes_generate[idx], cv::Scalar(idx * 1 % 255, idx * 5 % 255, idx * 10 % 255), 1);
		}
		cv::resize(img, img, cv::Size(img.cols * 0.5, img.rows * 0.5), 0, 0, cv::INTER_AREA);
		imshow("img_show", img);
		waitKey(0);
	}
}


void cropImg(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save)
{
	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u_choose(1, 3);

	// stage1
	std::vector<cv::Mat> img_rois;
	boxAug(img_aug, aug_parameter, img_rois, flag_img_show);

	// stage2
	int value_iter = aug_parameter.num_aug_max;
	aug_parameter.num_aug_max = 360 / aug_parameter.scale_angle;
	value_iter = value_iter / aug_parameter.num_aug_max;

	for (int idx_aug = 0; idx_aug < value_iter; idx_aug++)
	{
		std::pair<cv::Mat, xmlReadWrite> img_aug_affine = imgAug(img, annotation, "flag_affine");
		boxAug(img_aug_affine, aug_parameter, img_rois, flag_img_show);
		std::pair<cv::Mat, xmlReadWrite> img_aug_rotate = imgAug(img, annotation, "flag_rotate");
		boxAug(img_aug_rotate, aug_parameter, img_rois, flag_img_show);
	}

	// stage3
	std::vector<int> idx_box_fuse;
	for (int k = 0; k < img_rois.size(); k++)
	{
		idx_box_fuse.push_back(k);
	}
	std::random_shuffle(idx_box_fuse.begin(), idx_box_fuse.end());
	int box_fuse_ratio = (int)aug_parameter.value_fuse*img_rois.size();

	for (int idx_roi = 0; idx_roi < img_rois.size(); idx_roi++)
	{
		xmlReadWrite xml_tmp;
		cv::Mat img_crop = img_rois[idx_box_fuse[idx_roi]];

		if (idx_roi<box_fuse_ratio)
		{
			vector<string> flags_aug_rois;
			flags_aug_rois.push_back("flag_flip");
			flags_aug_rois.push_back("flag_noise");
			flags_aug_rois.push_back("flag_blur");
			//flags_aug_rois.push_back("flag_sharpen");
			std::random_shuffle(flags_aug_rois.begin(), flags_aug_rois.end());
			//flags_aug_rois.insert(flags_aug_rois.begin(), "flag_ori");
			int aug_choose = u_choose(e1);

			for (int idx_choose = 0; idx_choose < aug_choose; idx_choose++)
			{
				std::pair<cv::Mat, xmlReadWrite> img_aug = imgAug(img_crop, xml_tmp, flags_aug_rois[idx_choose]);
				img_crop = img_aug.first;
			}
		}

		if ((aug_parameter.height_crop) && (aug_parameter.width_crop))
		{
			cv::resize(img_crop, img_crop, cv::Size(aug_parameter.width_crop, aug_parameter.height_crop), 0, 0, cv::INTER_AREA);
		}

		if (flag_img_save)
		{
			std::string img_save = save_dir + R"(\)" + img_name.substr(0, img_name.size()-4) + "_aug_" + std::to_string(idx_roi) + ".jpg";
			cv::imwrite(img_save, img_crop, compression_params);
		}
	}
}

void cropImg2(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save)
{
	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u_choose(1, 3);

	// stage1:对原图上的目标生成扰动框
	std::vector<cv::Mat> img_rois;
	std::vector<int> label_rois;
	boxAug2(img_aug, aug_parameter, img_rois, label_rois, flag_img_show);

	// stage2:对原图先进行仿射和旋转，然后生成扰动框
	//int value_iter = aug_parameter.num_aug_max;
	//aug_parameter.num_aug_max = 360 / aug_parameter.scale_angle;
	//value_iter = value_iter / aug_parameter.num_aug_max;

	//for (int idx_aug = 0; idx_aug < value_iter; idx_aug++)
	//{
	//	std::pair<cv::Mat, xmlReadWrite> img_aug_affine = imgAug(img.clone(), annotation, "flag_affine");
	//	boxAug2(img_aug_affine, aug_parameter, img_rois, flag_img_show);
	//	std::pair<cv::Mat, xmlReadWrite> img_aug_rotate = imgAug(img.clone(), annotation, "flag_rotate");
	//	boxAug2(img_aug_rotate, aug_parameter, img_rois, flag_img_show);
	//}

	// stage3:对stage1和stage2生成的扰动框进行翻转、光照、噪声...等操作
	std::vector<int> idx_box_fuse;
	for (int k = 0; k < img_rois.size(); k++)
	{
		idx_box_fuse.push_back(k);
	}
	//std::random_shuffle(idx_box_fuse.begin(), idx_box_fuse.end());
	int box_fuse_ratio = (int)aug_parameter.value_fuse*img_rois.size();

	for (int idx_roi = 0; idx_roi < img_rois.size(); idx_roi++)
	{
		xmlReadWrite xml_tmp;
		cv::Mat img_crop = img_rois[idx_box_fuse[idx_roi]];

		if (idx_roi<box_fuse_ratio)
		{
			vector<string> flags_aug_rois;
			flags_aug_rois.push_back("flag_flip");
			flags_aug_rois.push_back("flag_noise");
			flags_aug_rois.push_back("flag_blur");
			//flags_aug_rois.push_back("flag_sharpen");
			std::random_shuffle(flags_aug_rois.begin(), flags_aug_rois.end());
			//flags_aug_rois.insert(flags_aug_rois.begin(), "flag_ori");
			int aug_choose = u_choose(e1);

			for (int idx_choose = 0; idx_choose < aug_choose; idx_choose++)
			{
				std::pair<cv::Mat, xmlReadWrite> img_aug = imgAug(img_crop, xml_tmp, flags_aug_rois[idx_choose]);
				img_crop = img_aug.first;
			}
		}

		if ((aug_parameter.height_crop) && (aug_parameter.width_crop))
		{
			cv::resize(img_crop, img_crop, cv::Size(aug_parameter.width_crop, aug_parameter.height_crop), 0, 0, cv::INTER_AREA);
		}

		if (flag_img_save)
		{
			createFolder(save_dir + std::to_string(label_rois[idx_roi]));
			std::string img_save = save_dir + std::to_string(label_rois[idx_roi]) + R"(\)" + img_name.substr(0, img_name.size() - 4) + "_aug_" + std::to_string(idx_roi) + ".jpg";
			cv::imwrite(img_save, img_crop, compression_params);


			//将图片翻转
			cv::Mat img_flip;
			cv::flip(img_crop, img_flip, 1);
			std::string img_save_flip = save_dir + std::to_string(label_rois[idx_roi]) + R"(\)" + img_name.substr(0, img_name.size() - 4) + "_aug_" + std::to_string(idx_roi) + "_flip" +".jpg";
			cv::imwrite(img_save_flip, img_flip, compression_params);



		}
	}
}
void cropImg2_longjump(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save, std::vector<cv::Point> innerpoints, std::vector<cv::Point> outerpoints)
{
	cv::Mat img = img_aug.first.clone();
	xmlReadWrite annotation = img_aug.second;

	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u_choose(1, 3);

	// stage1:对原图上的目标生成扰动框
	std::vector<cv::Mat> img_rois;
	std::vector<int> label_rois;
	boxAug2_longjump(img_aug, aug_parameter, img_rois, label_rois, flag_img_show, innerpoints, outerpoints);



	// stage2:对裁剪出来的小块进行常规的数据增强

	for (int idx_roi = 0; idx_roi < img_rois.size(); idx_roi++)
	{
		xmlReadWrite xml_tmp;
		cv::Mat img_crop = img_rois[idx_roi];


		
		int aug_index=9999;
		for (int idx_choose = 0; idx_choose < aug_flags.size(); idx_choose++)
		{
			
			std::pair<cv::Mat, xmlReadWrite> img_aug = imgAug(img_crop, xml_tmp, aug_flags[idx_choose]);
			cv::Mat img_aug_changgui=img_aug.first;
			aug_index = idx_choose;

			
			if ((aug_parameter.height_crop) && (aug_parameter.width_crop))
			{
				cv::resize(img_aug_changgui, img_aug_changgui, cv::Size(aug_parameter.width_crop, aug_parameter.height_crop), 0, 0, cv::INTER_AREA);
			}

			if (flag_img_save)
			{
				createFolder(save_dir + aug_parameter.obj_name_choose+ R"(\)" + aug_flags[aug_index]);
				std::string img_save = save_dir + aug_parameter.obj_name_choose + R"(\)" + aug_flags[aug_index] + R"(\)" + img_name.substr(0, img_name.size() - 4) + "_aug_" + std::to_string(idx_roi) + ".jpg";
				cv::imwrite(img_save, img_aug_changgui, compression_params);
			}
		}
		

	}
}

void save_img_xml_new_camera(std::string dst, std::string img_name, std::string flag_aug, std::pair<cv::Mat, xmlReadWrite> aug)
{
	// 裁剪尺寸
	const int cut_x = 1600;
	const int cut_y = 1300;

	// 图片质量
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	cv::Mat img_crop = aug.first(cv::Rect(0, 0, cut_y, cut_x));
	xmlReadWrite info_crop;
	for (size_t i = 0; i < aug.second.xml_object_vec.size(); i++)
	{
		cv::Rect original_box = aug.second.xml_object_vec[i].gt_boxes;
		std::vector<cv::Point> orignal_shapes = aug.second.xml_object_vec[i].gt_shapes;
		if ((original_box.x >= cut_y) || (original_box.y >= cut_x))
		{
			continue;
		}
		else
		{
			original_box.x = original_box.x > 0 ? original_box.x : 0;
			original_box.y = original_box.y > 0 ? original_box.y : 0;

			original_box.width = original_box.width + original_box.x > cut_y ? cut_y - original_box.x-1 : original_box.width;
			original_box.height = original_box.height + original_box.y > cut_x ? cut_x - original_box.y-1 : original_box.height;

			if ((original_box.width<2)|| (original_box.height<2))
			{
				continue;
			}

			xmlobject box_info;
			box_info.gt_boxes = original_box;
			std::vector<cv::Point> shapes_info;
			shapes_info.push_back(cv::Point(box_info.gt_boxes.x, box_info.gt_boxes.y));
			shapes_info.push_back(cv::Point(box_info.gt_boxes.x + box_info.gt_boxes.width, box_info.gt_boxes.y));
			shapes_info.push_back(cv::Point(box_info.gt_boxes.x + box_info.gt_boxes.width, box_info.gt_boxes.y + box_info.gt_boxes.height));
			shapes_info.push_back(cv::Point(box_info.gt_boxes.x, box_info.gt_boxes.y + box_info.gt_boxes.height));
			box_info.gt_shapes = shapes_info;
			box_info.box_name = aug.second.xml_object_vec[i].box_name;
			info_crop.xml_object_vec.push_back(box_info);
		}
	}

	if (info_crop.xml_object_vec.size())
	{
		std::string img_dst = dst + flag_aug + img_name;
		std::string xml_dst = img_dst.substr(0, img_dst.size() - 4) + ".xml";
		cv::imwrite(img_dst, img_crop, compression_params);
		writeXmlFile(xml_dst, flag_aug + img_name, info_crop);
	}


	//for (size_t i = 0; i < info_crop.xml_object_vec.size(); i++)
	//{
	//	cv::rectangle(img_crop, info_crop.xml_object_vec[i].gt_boxes, cv::Scalar(255, 0, 0), 2);
	//}
	//cv::resize(img_crop, img_crop, cv::Size(0.5*img_crop.cols, 0.5*img_crop.rows));
	//cv::imshow("img_crop", img_crop);
	//cv::waitKey(10);
}

void save_img_xml(std::string dst, std::string img_name, std::string flag_aug, std::pair<cv::Mat, xmlReadWrite> aug)
{
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	std::string img_dst = dst + flag_aug + img_name;
	std::string xml_dst = img_dst.substr(0, img_dst.size() - 4) + ".xml";
	cv::imwrite(img_dst, aug.first, compression_params);
	writeXmlFile(xml_dst, flag_aug + img_name, aug.second);

	cv::Mat img_crop = aug.first;

	//for (size_t i = 0; i < aug.second.xml_object_vec.size(); i++)
	//{
	//	cv::rectangle(img_crop, aug.second.xml_object_vec[i].gt_boxes, cv::Scalar(255, 0, 0), 2);
	//}
	//cv::resize(img_crop, img_crop, cv::Size(0.5*img_crop.cols, 0.5*img_crop.rows));
	//cv::imshow("img_crop", img_crop);
	//cv::waitKey(10);
}

/************************************************* 图像增广(只针对图像) *****************************************************/

cv::Mat imgAug(cv::Mat input_img, std::string flag_aug)
{
	cv::Mat img_src = input_img.clone();
	if (flag_aug == "flag_ori")
	{
		return img_src;
	}
	if (flag_aug == "flag_flip")
	{
		return img_Flip(img_src, 1);
	}
	if (flag_aug == "flag_noise")
	{
		return img_GaussianNoise(img_src);
	}
	if (flag_aug == "flag_blur")
	{
		return img_Blur(img_src);
	}
	if (flag_aug == "flag_sharpen")
	{
		return img_Sharpen(img_src);
	}
	if (flag_aug == "flag_affine")
	{
		return img_Affine(img_src, 0.01, 0.01);
	}
	if (flag_aug == "flag_rotate")
	{
		return img_Rotate(img_src, 0, 5);
	}
	if (flag_aug == "flag_gamma")
	{
	
		return img_Gamma(input_img,  0.7, 0.9, 0.8, false);
	}
	if (flag_aug == "flag_illum")
	{
		return img_Illum_lighting(input_img,  100, 110, 100, 150, "gaussian");
	}

}



cv::Mat img_Affine(cv::Mat input_img, float affine_x, float affine_y)
{
	// 存储结果
	cv::Mat output_img;

	std::random_device r;
	default_random_engine e1(r());

	int Affine_xa = affine_x * input_img.cols*(-1);
	int Affine_xb = affine_x * input_img.cols;

	int Affine_ya = affine_y * input_img.rows*(-1);
	int Affine_yb = affine_y * input_img.rows;

	uniform_int_distribution<unsigned> u0(0, Affine_xb - Affine_xa + 1);
	uniform_int_distribution<unsigned> u1(0, Affine_yb - Affine_ya + 1);


	int Affine_value_x1 = u0(e1) + Affine_xa;
	int Affine_value_x2 = u0(e1) + Affine_xa;
	int Affine_value_x3 = u0(e1) + Affine_xa;
	int Affine_value_x4 = u0(e1) + Affine_xa;


	int Affine_value_y1 = u1(e1) + Affine_ya;
	int Affine_value_y2 = u1(e1) + Affine_ya;
	int Affine_value_y3 = u1(e1) + Affine_ya;
	int Affine_value_y4 = u1(e1) + Affine_ya;

	cv::Point2f AffinePoints0[4] = { cv::Point2f(0, 0), cv::Point2f(0, input_img.rows), cv::Point2f(input_img.cols, 0), cv::Point2f(input_img.cols, input_img.rows) };
	cv::Point2f AffinePoints1[4] = { cv::Point2f(Affine_value_x1, Affine_value_y1), cv::Point2f(Affine_value_x2, input_img.rows - Affine_value_y2), cv::Point2f(input_img.cols - Affine_value_x3, Affine_value_y3), cv::Point2f(input_img.cols - Affine_value_x4, input_img.rows - Affine_value_y4) };

	cv::Mat Trans = cv::getAffineTransform(AffinePoints0, AffinePoints1);
	warpAffine(input_img, output_img, Trans, cv::Size(input_img.cols, input_img.rows), CV_INTER_CUBIC);

	return output_img;
}

cv::Mat img_Blur(cv::Mat input_img)
{
	// 存储结果
	cv::Mat output_img;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u0(0, 2);

	//int size[] = { 3, 5, 7, 9 };
	//int size[] = { 3, 3, 3 };
	int size[] = { 3, 5, 7 };
	int size_index = u0(e1);
	cv::GaussianBlur(input_img, output_img, cv::Size(size[size_index], size[size_index]), 2.0);

	return output_img;
}

cv::Mat img_Blur_Kernel(cv::Mat input_img, int blur_kernel_size)
{
	// 存储结果
	cv::Mat output_img;
	cv::GaussianBlur(input_img, output_img, cv::Size(blur_kernel_size, blur_kernel_size), 2.0);
	return output_img;
}

cv::Mat img_ChannelShift(cv::Mat input_img)
{
	cv::Mat output_img;
	if (input_img.channels() == 3)
	{
		std::vector<cv::Mat> img_channels(3);
		cv::split(input_img, img_channels);
		std::random_shuffle(img_channels.begin(), img_channels.end());
		cv::merge(img_channels, output_img);
	}
	else
	{
		output_img = input_img;
	}

	return output_img;
}

cv::Mat img_ContrastAug(cv::Mat input_img, float alpha, float beta, bool flag_fix)
{
	/*
	m     – 目标矩阵。如果m在运算前没有合适的尺寸或类型，将被重新分配。

	rtype – 目标矩阵的类型。因为目标矩阵的通道数与源矩阵一样，所以rtype也可以看做是目标矩阵的位深度。如果rtype为负值，目标矩阵和源矩阵将使用同样的类型。

	alpha – 尺度变换因子（可选）。

	beta  – 附加到尺度变换后的值上的偏移量（可选）。
	"*/

	// 存储结果
	cv::Mat output_img;

	float value_alpha = 0;
	float value_bate = 0;

	if (flag_fix)
	{
		value_alpha = alpha;
		value_bate = beta;
	}
	else
	{
		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u1(0, 9);

		value_alpha = alpha + u1(e1) / 100.0;
		value_bate = beta + u1(e1) / 100.0;
	}

	input_img.convertTo(output_img, -1, value_alpha, value_bate);// 参数可调

	return output_img;
}

cv::Mat img_Flip(cv::Mat input_img, int flag_flip)
{
	cv::Mat output_img;
	cv::flip(input_img, output_img, flag_flip);
	return output_img;
}

cv::Mat img_Gamma(cv::Mat input_img, float gamma_factor_min, float gamma_factor_max, float famma_factor_fix, bool flag_fix)
{
	// 存储结果
	cv::Mat output_img;

	float value_gamma = 0;

	if (flag_fix)
	{
		value_gamma = famma_factor_fix;
	}
	else
	{
		std::random_device r;
		default_random_engine e1(r());
		uniform_int_distribution<unsigned> u1(gamma_factor_min * 100, gamma_factor_max * 100);

		value_gamma = u1(e1) / 100.0;
	}


	// 建表
	unsigned char lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), value_gamma)*255.0f);
	}

	output_img = input_img.clone();
	const int img_channels = output_img.channels();

	switch (img_channels)
	{
	case 1:
	{
		cv::MatIterator_<uchar> it, end;
		for (it = output_img.begin<uchar>(), end = output_img.end<uchar>(); it != end; it++)
		{
			*it = lut[(*it)];
		}
		break;
	}
	case 3:
	{
		cv::MatIterator_<Vec3b> it3, end3;
		for (it3 = output_img.begin<Vec3b>(), end3 = output_img.end<Vec3b>(); it3 != end3; it3++)
		{
			(*it3)[0] = lut[((*it3)[0])];
			(*it3)[1] = lut[((*it3)[1])];
			(*it3)[2] = lut[((*it3)[2])];
		}
		break;
	}
	default:
	{
		std::cout << "img channel is wrong! plz check!" << std::endl;
		break;
	}
	}
	return output_img;
}

cv::Mat img_GaussianNoise(cv::Mat input_img)
{
	// 存储结果
	cv::Mat output_img;

	output_img = input_img.clone();
	//int Noise_s_value[] = { 3, 4, 5, 6, 7 };
	//int Noise_a = 0;
	//int Noise_b = 4;
	//int Noise_index = (rand() % (Noise_b - Noise_a + 1)) + Noise_a;

	//std::random_device rd;
	//std::mt19937 gen(rd());
	//std::normal_distribution<> d(0, Noise_s_value[Noise_index]);

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<unsigned> u1(3, 7);


	auto rows = output_img.rows;
	auto cols = output_img.cols * output_img.channels();

	for (int i = 0; i < rows; i++)
	{
		auto p = output_img.ptr<uchar>(i);
		for (int j = 0; j < cols; j++)
		{
			auto tmp = p[j] + u1(e1);
			tmp = tmp > 255 ? 255 : tmp;
			tmp = tmp < 0 ? 0 : tmp;
			p[j] = tmp;
		}
	}

	return output_img;
}

cv::Mat img_HsvJitter(cv::Mat input_img, float hue, float saturation, float exposure)
{
	// 存储结果
	cv::Mat output_img;

	// 参考darknet框架中的image.c实现
	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<signed> u0(0, 100);
	uniform_int_distribution<signed> u1(-hue * 100, hue * 100);
	uniform_int_distribution<signed> u2(saturation * 100, 100);
	uniform_int_distribution<signed> u3(exposure * 100, 100);

	float value_hue = u1(e1);
	float value_sat, value_exp;
	if (u0(e1) % 2)
	{
		value_sat = u2(e1) / 100.0;
	}
	else
	{
		value_sat = 100. / u2(e1);
	}
	if (u0(e1) % 2)
	{
		value_exp = u3(e1) / 100.0;
	}
	else
	{
		value_exp = 100. / u3(e1);
	}

	cv::Mat img_hsv;
	cv::cvtColor(input_img, img_hsv, CV_BGR2HSV);

	std::vector<cv::Mat> channels_hsv, channels_hsv_refine;
	cv::split(img_hsv, channels_hsv);

	cv::Mat hsv_H, hsv_S, hsv_V;
	hsv_H = channels_hsv.at(0);
	hsv_S = channels_hsv.at(1);
	hsv_V = channels_hsv.at(2);

	hsv_H = hsv_H + value_hue;
	hsv_S = hsv_S*value_sat;
	hsv_V = hsv_V*value_exp;

	channels_hsv_refine.push_back(hsv_H);
	channels_hsv_refine.push_back(hsv_S);
	channels_hsv_refine.push_back(hsv_V);

	cv::merge(channels_hsv_refine, output_img);

	cv::cvtColor(output_img, output_img, CV_HSV2BGR);

	return output_img;
}

cv::Mat img_Illum_lighting(cv::Mat input_img, float power_min, float power_max, float scale_min, float scale_max, std::string light_type)
{
	// 存储结果
	cv::Mat output_img;

	int im_w = input_img.cols;
	int im_h = input_img.rows;
	int im_c = input_img.channels();

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(-50, 50);
	uniform_int_distribution<unsigned> u1(power_min, power_max);
	uniform_int_distribution<unsigned> u2(scale_min, scale_max);


	int center_x = 0.5*input_img.cols;
	int center_y = 0.5*input_img.rows;

	cv::Point value_center(center_x + u0(e1), center_y + u0(e1));

	cv::Mat res_map;
	cv::Mat img_distance = illum_Get_Distance(input_img, value_center);

	if (light_type == "gaussian")
	{
		res_map = illum_Get_Guassian_Map(img_distance, u1(e1) / 100.0, u2(e1) / 100.0, im_w, im_h);
	}
	else if (light_type == "linear")
	{
		res_map = illum_Get_Linear_Map(img_distance, u1(e1) / 100.0, u2(e1) / 100.0, im_w, im_h);
	}
	else
	{
		std::cout << "Light_type Error!" << std::endl;
		exit(1);
	}

	cv::Mat img_res_map;
	cv::Mat Img_src_float;

	if (im_c == 3)
	{
		std::vector<cv::Mat> img_channels;
		img_channels.push_back(res_map);
		img_channels.push_back(res_map);
		img_channels.push_back(res_map);
		cv::merge(img_channels, img_res_map);
		input_img.convertTo(Img_src_float, CV_32FC3, 1, 0);
	}
	else
	{
		img_res_map = res_map.clone();
		input_img.convertTo(Img_src_float, CV_32FC1, 1, 0);

	}

	cv::Mat res_img = Img_src_float.mul(img_res_map);

	if (im_c == 3)
	{
		res_img.convertTo(output_img, CV_8UC3, 1, 0);
	}
	else
	{
		res_img.convertTo(output_img, CV_8UC1, 1, 0);
	}

	return output_img;
}

cv::Mat img_LogAug(cv::Mat input_img, float param1, float param2)
{
	// 存储结果
	cv::Mat output_img;

	if (input_img.channels()<3)
	{
		output_img = cv::Mat(input_img.size(), CV_32FC1);
	}
	else
	{
		output_img = cv::Mat(input_img.size(), CV_32FC3);
	}

	float *logtable = new float[256];
	for (int i = 0; i < 256; i++)
	{
		logtable[i] = log((float)i *param1 + param2);
	}
	for (int i = 0; i < input_img.rows; i++)
	{
		uchar * ptrg = input_img.ptr<uchar>(i);
		float * ptr = output_img.ptr<float>(i);
		for (int j = 0; j < output_img.cols*input_img.channels(); j++)
		{
			ptr[j] = logtable[ptrg[j]];
		}
	}
	delete[] logtable;
	normalize(output_img, output_img, 0, 255, CV_MINMAX);

	//转换成8bit图像显示    
	convertScaleAbs(output_img, output_img);

	return output_img;
}

cv::Mat img_PCAJitter(cv::Mat input_img)
{
	// 存储结果
	cv::Mat output_img;
	std::pair<cv::Mat, xmlReadWrite> output;

	std::random_device r;
	default_random_engine e1(r());
	uniform_int_distribution<signed> u0(0, 100);

	return output_img;
}

cv::Mat img_Rotate(cv::Mat input_img, int rotate_angle_start, int rotate_angle_end)
{
	// 存储结果
	cv::Mat output_img;

	std::random_device r;
	default_random_engine e1(r());

	uniform_int_distribution<unsigned> u0(0, 50);
	uniform_int_distribution<unsigned> u1(rotate_angle_start, rotate_angle_end);
	uniform_int_distribution<unsigned> u2(0, 9);

	int center_x = 0.5*input_img.cols;
	int center_y = 0.5*input_img.rows;

	cv::Point2f rotate_center(center_x + u0(e1), center_y + u0(e1));

	int flag_rotate = u2(e1) >= 5 ? 1 : -1;

	int rotate_angle = u1(e1)*flag_rotate;

	cv::Mat rotMat = cv::getRotationMatrix2D(rotate_center, rotate_angle, 1);
	cv::warpAffine(input_img, output_img, rotMat, input_img.size());

	return output_img;
}

cv::Mat img_Sharpen(cv::Mat input_img)
{
	// 存储结果
	cv::Mat output_img;

	//创建并初始化滤波模板
	cv::Mat kernel(3, 3, CV_32F, cv::Scalar(0));
	kernel.at<float>(1, 1) = 5.0;
	kernel.at<float>(0, 1) = -1.0;
	kernel.at<float>(1, 0) = -1.0;
	kernel.at<float>(1, 2) = -1.0;
	kernel.at<float>(2, 1) = -1.0;

	output_img = cv::Mat(input_img.size(), input_img.type());

	//对图像进行滤波
	cv::filter2D(input_img, output_img, input_img.depth(), kernel);
	return output_img;
}