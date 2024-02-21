#pragma warning(disable:4996)

#ifndef GET_XML_BOXES_H_
#define GET_XML_BOXES_H_

#include "GetXmlBoxes.h"
#include "zupply.hpp"
/*
遍历xml文件夹，找到所有的xml文件
*/
void GetPicFromXmlBox::readXmlFiles()
{
	filesNameXml.clear(); //清空
	std::string xmlFileSearch = xmlDir + "\\*.xml";
	//文件存储信息结构体 
	struct _finddata_t fileinfo;
	//保存文件句柄 
	intptr_t fHandle;

	fHandle = _findfirst(xmlFileSearch.c_str(), &fileinfo);

	if (-1LL == fHandle)
	{
		std::cout << "no files in " << xmlFileSearch << std::endl;
	}
	else
	{
		do
		{
			filesNameXml.push_back(fileinfo.name);
		} while (_findnext(fHandle, &fileinfo) == 0);
	}
	_findclose(fHandle);
}

int ii = 0;

/*
从一个XML文件中，找到所有的points
*/
void GetPicFromXmlBox::GetBoxesFromAXml(const std::string & filesNameXml)
{
	boxes_contours_with_label.clear();  //清空框
	std::pair<std::vector<cv::Point2i>, std::string> aBoxPair;
	TiXmlDocument mydoc;  // XML对象  
	std::string xmlFilePath = xmlDir + "\\" + filesNameXml;
	std::string box_label;

	if (!mydoc.LoadFile(xmlFilePath.c_str()))
	{
		std::cout << "Error: Could not load the xml file." << mydoc.ErrorDesc() << std::endl;
		exit(1);
	}

	// 根元素
	TiXmlElement *RootElement = mydoc.RootElement();
	TiXmlElement *pEle = RootElement;

	// 遍历该结点  循环object那一列  
	for (TiXmlElement *firstColElement = pEle->FirstChildElement(); firstColElement != NULL; \
		firstColElement = firstColElement->NextSiblingElement())
	{
		// 标注目标
		if (stricmp(firstColElement->Value(), "object") == 0)
		{
			// 获取 contour
			std::vector<cv::Point2i> contour;
			/* 循环name那一列 */
			for (TiXmlElement *secondColElement = firstColElement->FirstChildElement(); \
				secondColElement != NULL; secondColElement = secondColElement->NextSiblingElement())
			{
				// 判定该目标名称是否为所选目标
				if (stricmp(secondColElement->Value(), "name") == 0)
				{
					bool for_flag = true;
					for (int _i_ = 0; _i_ < box_labels.size(); _i_++)
					{
						if (stricmp(secondColElement->FirstChild()->Value(), box_labels[_i_].c_str()) == 0)
						{
							box_label = secondColElement->FirstChild()->Value();
							for_flag = !for_flag;
							break;   
						}					
					}
					if (for_flag)
					{
						std::cout << "no label of: " << secondColElement->FirstChild()->Value() << std::endl;
						break;    //未知标签，不予存储
					}
				}
				if (stricmp(secondColElement->Value(), "shape") == 0)
				{
					for (TiXmlElement *threeElement1 = secondColElement->FirstChildElement(); threeElement1 != NULL; \
						threeElement1 = threeElement1->NextSiblingElement())
					{
						if (stricmp(threeElement1->Value(), "points") == 0)
						{
							cv::Point2i now_point;
							for (TiXmlElement *sonElement = threeElement1->FirstChildElement(); sonElement != NULL; \
								sonElement = sonElement->NextSiblingElement())
							{
								if (stricmp(sonElement->Value(), "x") == 0)
								{
									now_point.x = std::stoi(sonElement->FirstChild()->Value());
								}
								sonElement = sonElement->NextSiblingElement();
								if (stricmp(sonElement->Value(), "y") == 0)
								{
									now_point.y = std::stoi(sonElement->FirstChild()->Value());
								}
								contour.push_back(now_point);
							}

						}

					}

					aBoxPair.first = contour;
					aBoxPair.second = box_label;
					boxes_contours_with_label.push_back(aBoxPair);
				}
				// 获取 points
				
			}
		}
	}
	//std::cout << ii++ << " : filesNameXml: " << filesNameXml << " contours_with_label_num: " << boxes_contours_with_label.size() << std::endl;
}

/*
把当前所有的Boxes进行txt的存储
*/
void GetPicFromXmlBox::GeneratePicFromBoxes(const std::string & fileName)
{
	char boxName[20] = { 0 };

	std::string saveTxtName = saveTxtDir + "\\" + fileName + ".txt";
	std::ofstream saveTxt(saveTxtName, std::ios::out);
	if (!saveTxt) std::cerr << "Open Txt file error!" << std::endl;

	for (auto aboxLabel : boxes_contours_with_label)
	{
		for (auto point : aboxLabel.first)
		{
			saveTxt << point.x << " " << point.y << " ";     //写点
		}
		saveTxt << aboxLabel.second << " 0" << "\n";         //标签 识别困难程度（默认0）
	}

	saveTxt.close();
	//std::cout << "Saved " << saveTxtName << std::endl;
	
}

void GetPicFromXmlBox::AutoRunning(void)
{
	std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);
	std::cout << std::setw(15) << "xmlDir: " << xmlDir << std::endl;
	std::cout << std::setw(15) << "saveTxtDir: " << saveTxtDir << std::endl;

	readXmlFiles();
	std::cout << std::endl << "xmlFilesNum: " << filesNameXml.size() << std::endl << std::endl;
	zz::log::ProgBar pbar(filesNameXml.size(), "Processing: ");
	for (auto xmlFile : filesNameXml)
	{
		std::string fileName;
		GetBoxesFromAXml(xmlFile);
		fileName = xmlFile.substr(0, xmlFile.length() - 4);
		std::cout << "xmlFile: "<< xmlFile << std::endl;
		GeneratePicFromBoxes(fileName);
		pbar.step(1);
	}
	std::cout << "All have done!" << std::endl;
}

#endif 


