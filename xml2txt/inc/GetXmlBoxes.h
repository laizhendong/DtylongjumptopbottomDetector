#ifndef GET_XML_BOXES_
#define GET_XML_BOXES_

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <tinyxml.h> 
#include <iomanip>

#include <direct.h>
#include <io.h>
#include <Windows.h>

class GetPicFromXmlBox
{
private:
	std::vector<std::pair<std::vector<cv::Point2i>, std::string>> boxes_contours_with_label;  //一个xml中所有带有标签的轮廓
	std::vector<std::string> filesNameXml;
	std::string xmlDir;
	std::string saveTxtDir;
	std::vector<std::string> box_labels;   //标签

public:
	GetPicFromXmlBox() {}

	void XmlDir(const std::string xml_Dir) { this->xmlDir = xml_Dir; }
	void TxtSaveDir(const std::string saveTxtDir) { this->saveTxtDir = saveTxtDir; }
	void BoxLabel(std::vector<std::string> box_labels) { this->box_labels = box_labels; }

	void AutoRunning(void);
	void GetBoxesFromAXml(const std::string & filesNameXml);
	void readXmlFiles(void);
	void GeneratePicFromBoxes(const std::string & fileName);
};

#endif

