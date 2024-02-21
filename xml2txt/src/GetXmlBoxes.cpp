#pragma warning(disable:4996)

#ifndef GET_XML_BOXES_H_
#define GET_XML_BOXES_H_

#include "GetXmlBoxes.h"
#include "zupply.hpp"
/*
����xml�ļ��У��ҵ����е�xml�ļ�
*/
void GetPicFromXmlBox::readXmlFiles()
{
	filesNameXml.clear(); //���
	std::string xmlFileSearch = xmlDir + "\\*.xml";
	//�ļ��洢��Ϣ�ṹ�� 
	struct _finddata_t fileinfo;
	//�����ļ���� 
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
��һ��XML�ļ��У��ҵ����е�points
*/
void GetPicFromXmlBox::GetBoxesFromAXml(const std::string & filesNameXml)
{
	boxes_contours_with_label.clear();  //��տ�
	std::pair<std::vector<cv::Point2i>, std::string> aBoxPair;
	TiXmlDocument mydoc;  // XML����  
	std::string xmlFilePath = xmlDir + "\\" + filesNameXml;
	std::string box_label;

	if (!mydoc.LoadFile(xmlFilePath.c_str()))
	{
		std::cout << "Error: Could not load the xml file." << mydoc.ErrorDesc() << std::endl;
		exit(1);
	}

	// ��Ԫ��
	TiXmlElement *RootElement = mydoc.RootElement();
	TiXmlElement *pEle = RootElement;

	// �����ý��  ѭ��object��һ��  
	for (TiXmlElement *firstColElement = pEle->FirstChildElement(); firstColElement != NULL; \
		firstColElement = firstColElement->NextSiblingElement())
	{
		// ��עĿ��
		if (stricmp(firstColElement->Value(), "object") == 0)
		{
			// ��ȡ contour
			std::vector<cv::Point2i> contour;
			/* ѭ��name��һ�� */
			for (TiXmlElement *secondColElement = firstColElement->FirstChildElement(); \
				secondColElement != NULL; secondColElement = secondColElement->NextSiblingElement())
			{
				// �ж���Ŀ�������Ƿ�Ϊ��ѡĿ��
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
						break;    //δ֪��ǩ������洢
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
				// ��ȡ points
				
			}
		}
	}
	//std::cout << ii++ << " : filesNameXml: " << filesNameXml << " contours_with_label_num: " << boxes_contours_with_label.size() << std::endl;
}

/*
�ѵ�ǰ���е�Boxes����txt�Ĵ洢
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
			saveTxt << point.x << " " << point.y << " ";     //д��
		}
		saveTxt << aboxLabel.second << " 0" << "\n";         //��ǩ ʶ�����ѳ̶ȣ�Ĭ��0��
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


