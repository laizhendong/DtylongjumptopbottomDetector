#include "Facevisa_DtyLongjumpTopBottomDetector.h"
#include "KPDTYBottombright.h"
#include "KPDTYTopbright.h"
#include "KPDTYSidetopborderbright.h"
#include "KPDTYSidebottomborderbright.h"
#include "Cascade_util.h"
#ifdef WIN32
#include <Windows.h>           //GetModuleFileNameA    MAX_PATH
#include <direct.h>            //_mkdir
#else
#include <unistd.h>            //readlink
#include <sys/stat.h>          //mkdir
#include <dirent.h>
#endif

using namespace std;
using namespace cv;
#define MAX_PATH (1024) 
#define BUFFER_LENGTH (1024)

//SDK测试
#define SDK_TEST			       (1)
#define PREDICTION			       (1)       //前向测试
#define flag_save_xml_img_det	   (0)       // 检测器保存xml
#define flag_save_xml_img_cls	   (0)       // 分类器保存xml
#define flag_show_detection		   (1)       // mark图保存
#define QA_shujishouji             (0)       //QA收集分类器漏检数据收集专用
#define Show_landmark              (0)       //关键点显示

static LongjumpTopBottomHandle handle = 0;

//linux下读入图片
#ifndef WIN32

int ReadImagePath_(std::string path, std::vector<std::string>& files)
{
	std::cout << "image folder :  " << path << std::endl;
	struct dirent *direntp;
	DIR *dirp = opendir(path.c_str());

	if (dirp != NULL)
	{
		std::cout << "open dir success" << std::endl;
		while ((direntp = readdir(dirp)) != NULL)
		{
			if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0)    ///current dir OR parrent dir
				continue;
			std::cout << "before push file" << std::endl;
			std::cout << "file name: " << direntp->d_name << std::endl;
			files.push_back(direntp->d_name);
		}
		std::cout << "files number: " << files.size() << std::endl;
		//printf("%s\n", direntp->d_name);
	}
	else
	{
		std::cout << "open dir failed" << std::endl;
	}
	closedir(dirp);
	return 0;
}
#endif

//linux复制文件
#ifndef WIN32
void CopyFile(std::string sourcePath, std::string destPath, bool FLAG)//Windows的copyfile函数在linux下没有，要重新定义
{
	int len = 0;
	FILE *pIn = NULL;
	FILE *pOut = NULL;
	char buff[BUFFER_LENGTH] = { 0 };

	if ((pIn = fopen(sourcePath.c_str(), "r")) == NULL)
	{
		printf("Open File %s Failed...\n", sourcePath.c_str());
		//return 1;
	}
	if ((pOut = fopen(destPath.c_str(), "w")) == NULL)
	{
		printf("Create Dest File Failed...\n");
		//return 1;
	}

	while ((len = fread(buff, sizeof(char), sizeof(buff), pIn))>0)
	{
		fwrite(buff, sizeof(char), len, pOut);
	}
	fclose(pOut);
	fclose(pIn);
}


#endif

int checkEmpty(cv::Mat image) 
{
	cv::Scalar mean_val = mean(image);
	double mean_b = mean_val[0];
	double mean_g = mean_val[1];
	double mean_r = mean_val[2];
	if ((mean_b < 30) && (mean_g < 30) && (mean_g < 30)) {
		std::cout << "空盘！" << std::endl;
		return 1;
	}
	return 0;
}

#ifdef WIN32
bool DTY_Bottom_Crop_Full_img(std::string img_name, cv::Mat img_src, cv::Rect rect_cut, xmlReadWrite img_info, std::string dir_save, std::vector<std::string> flags_aug)
{
	// 获取shapes点边界极值
	std::vector<int> location_x, location_y;
	for (auto idx_shape = 0; idx_shape < img_info.xml_object_vec.size(); idx_shape++)
	{
		for (auto idx_point = 0; idx_point < img_info.xml_object_vec[idx_shape].gt_shapes.size(); idx_point++)
		{
			location_x.push_back(img_info.xml_object_vec[idx_shape].gt_shapes[idx_point].x);
			location_y.push_back(img_info.xml_object_vec[idx_shape].gt_shapes[idx_point].y);
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

	// 随机位置
	vector<int> move_pixel_x1 = { -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	vector<int> move_pixel_y1 = { -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	vector<int> move_pixel_x2 = { -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	vector<int> move_pixel_y2 = { -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

	random_shuffle(move_pixel_x1.begin(), move_pixel_x1.end());
	random_shuffle(move_pixel_y1.begin(), move_pixel_y1.end());
	random_shuffle(move_pixel_x2.begin(), move_pixel_x2.end());
	random_shuffle(move_pixel_y2.begin(), move_pixel_y2.end());

	move_pixel_x1.insert(move_pixel_x1.begin(), 0);
	move_pixel_y1.insert(move_pixel_y1.begin(), 0);
	move_pixel_x2.insert(move_pixel_x2.begin(), 0);
	move_pixel_y2.insert(move_pixel_y2.begin(), 0);

	cv::Mat roi = img_src(rect_cut);
	cv::Mat img_illum = img_src.clone();

	for (auto idx = 0; idx < 6; idx++)  //crop扰动
	{
		auto cut_x1 = rect_cut.x + 2 * move_pixel_x1[idx] < 0 ? 0 : rect_cut.x + 2 * move_pixel_x1[idx];
		auto cut_y1 = rect_cut.y + 2 * move_pixel_y1[idx] < 0 ? 0 : rect_cut.y + 2 * move_pixel_y1[idx];
		auto cut_x2 = rect_cut.x + rect_cut.width + 2 * move_pixel_x2[idx] > img_src.cols ? img_src.cols : rect_cut.x + rect_cut.width + 2 * move_pixel_x2[idx];
		auto cut_y2 = rect_cut.y + rect_cut.height + 2 * move_pixel_y2[idx] > img_src.rows ? img_src.rows : rect_cut.y + rect_cut.height + 2 * move_pixel_y2[idx];

		if (cut_x1 >= shape_x_min)
		{
			cut_x1 = max(0, shape_x_min - 3);
		}
		if (cut_y1 >= shape_y_min)
		{
			cut_y1 = max(0, shape_y_min - 3);
		}
		if (cut_x2 <= shape_x_max)
		{
			cut_x2 = min(img_src.cols, shape_x_max + 3);
		}
		if (cut_y2 <= shape_y_max)
		{
			cut_y2 = min(img_src.rows, shape_y_max + 3);
		}

		// 裁剪
		cv::Mat img_cut = img_src(cv::Range(cut_y1, cut_y2), cv::Range(cut_x1, cut_x2));

		// 调整xml
		xmlReadWrite img_info_new;
		img_info_new.img_depth = img_info.img_depth;
		img_info_new.img_score = img_info.img_score;
		img_info_new.img_width = cut_x2 - cut_x1;
		img_info_new.img_hight = cut_y2 - cut_y1;

		for (auto idx_obj = 0; idx_obj < img_info.xml_object_vec.size(); idx_obj++)
		{
			xmlobject obj_new;
			obj_new.box_name = img_info.xml_object_vec[idx_obj].box_name;
			obj_new.gt_level = img_info.xml_object_vec[idx_obj].gt_level;
			obj_new.gt_boxes.x = img_info.xml_object_vec[idx_obj].gt_boxes.x - cut_x1;
			obj_new.gt_boxes.y = img_info.xml_object_vec[idx_obj].gt_boxes.y - cut_y1;
			obj_new.gt_boxes.width = img_info.xml_object_vec[idx_obj].gt_boxes.width;
			obj_new.gt_boxes.height = img_info.xml_object_vec[idx_obj].gt_boxes.height;
			for (auto shape_idx = 0; shape_idx < img_info.xml_object_vec[idx_obj].gt_shapes.size(); shape_idx++)
			{
				cv::Point point_new;
				point_new.x = img_info.xml_object_vec[idx_obj].gt_shapes[shape_idx].x - cut_x1;
				point_new.y = img_info.xml_object_vec[idx_obj].gt_shapes[shape_idx].y - cut_y1;
				obj_new.gt_shapes.push_back(point_new);
			}
			img_info_new.xml_object_vec.push_back(obj_new);
		}

		// 图像质量
		std::vector<int> compression_params;
		compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
		compression_params.push_back(100);

		// 数据扩增
		if (idx == 0)
		{
			for (auto idx_aug = 0; idx_aug < flags_aug.size(); idx_aug++)
			{
				cv::Mat img_aug = imgAug(roi, flags_aug[idx_aug]);
				std::string img_save = dir_save + flags_aug[idx_aug] + "/" + flags_aug[idx_aug] + "_" + img_name.substr(0, img_name.size() - 4) + "_crop_" + to_string(idx) + ".jpg";
				std::string xml_save = img_save.substr(0, img_save.size() - 4) + ".xml";
				if (flags_aug[idx_aug] == "flag_flip")
				{
					xmlReadWrite img_info_new_flip;
					flipXmlFile(img_info_new, img_info_new_flip);

					// 处理越界
					if (!checkXmlFile(img_info_new_flip))
					{
						continue;
					}
					else
					{
						cv::imwrite(img_save, img_aug, compression_params);
						writeXmlFile(xml_save, img_name, img_info_new_flip);
					}
				}
				else if (flags_aug[idx_aug] == "flag_illum")
				{
					// 原图光照
					default_random_engine e3, e4;
					uniform_int_distribution<unsigned> u3(int(0.33*img_illum.cols), int(0.66*img_illum.cols));
					uniform_int_distribution<unsigned> u4(500, 1000);

					for (int i = 0; i < 2; i++) //光照扰动次数
					{
						cv::Point point;
						point.x = u3(e3);
						point.y = u4(e4);

						cv::Mat img_illum1 = img_Illum_lighting(img_illum.clone(), 90, 150, 90, 130, "gaussian");

						cv::Mat img_cut = img_illum1(cv::Range(cut_y1, cut_y2), cv::Range(cut_x1, cut_x2));

						std::string img_save_illum1 = dir_save + flags_aug[idx_aug] + "/" + flags_aug[idx_aug] + "_" + img_name.substr(0, img_name.size() - 4) + "_crop_" + to_string(i) + ".jpg";
						std::string xml_save_illum1 = img_save_illum1.substr(0, img_save.size() - 4) + ".xml";
						cv::imwrite(img_save_illum1, img_cut, compression_params);
						writeXmlFile(xml_save_illum1, img_name, img_info_new);
					}
				}
				else
				{
					// 处理越界
					if (!checkXmlFile(img_info_new))
					{
						continue;
					}
					else
					{
						cv::imwrite(img_save, img_aug, compression_params);
						writeXmlFile(xml_save, img_name, img_info_new);
					}
				}
			}
		}
		else
		{
			// 处理越界
			if (!checkXmlFile(img_info_new))
			{
				continue;
			}
			else
			{

				createFolder(dir_save + "flag_crop/");
				std::string img_save = dir_save + "flag_crop/" + "flag_crop" + "_" + img_name.substr(0, img_name.size() - 4) + "_crop_" + to_string(idx) + ".jpg";
				std::string xml_save = img_save.substr(0, img_save.size() - 4) + ".xml";
				cv::imwrite(img_save, img_cut, compression_params);
				writeXmlFile(xml_save, img_name, img_info_new);
			}
		}
	}
	return 0;     ////////*********************************************************
}
//王翀源码，没改
cv::Rect DTY_generateBoundary_Bottom(cv::Mat srcimg, cv::Mat gray, std::vector<cv::Point> innerpoints, std::vector<cv::Point> externalpoints, \
	cv::Mat &ROI_img, cv::Mat &ROI_gray, cv::Mat & roi, cv::Vec3f& circ, int & left, int & top, int& cutx, cv::Mat & roi_InnerBoundary)
{
	int isvalid = 1;
	roi = cv::Mat(gray.rows, gray.cols, CV_8UC1, cv::Scalar::all(255));

	vector<cv::Point> tmppoints;
	for (int i = 0, le = innerpoints.size(); i < le; i++)
	{
		if (i == 0)
		{
			if (innerpoints[i].y > 0)
			{
				tmppoints.push_back(cv::Point(innerpoints[i].x >> 1, 0));
			}
		}
		else if (i == le - 1)
		{
			if (innerpoints[i].y >0)
			{
				tmppoints.push_back(cv::Point(innerpoints[i].x >> 1, 0));
			}
		}
		else
		{
			tmppoints.push_back(cv::Point(innerpoints[i].x >> 1, innerpoints[i].y >> 1));
		}
	}

	for (int i = 0, le = tmppoints.size(); i < le - 1; i++)
	{
		for (int x = tmppoints[i].x; x <= tmppoints[i + 1].x; x++)
		{
			int endy;

			if (tmppoints[i].x == tmppoints[i + 1].x)
			{
				continue;
			}

			endy = (tmppoints[i].y - tmppoints[i + 1].y)*(x - tmppoints[i].x) / ((tmppoints[i].x - tmppoints[i + 1].x)) + tmppoints[i].y;

			if (endy > roi.rows - 1)
			{
				endy = roi.rows - 1;
			}

			for (int y = 0; y < endy; y++)
			{
				roi.at<uchar>(y, x) = 0;
			}
		}
	}

	tmppoints.clear();
	for (int i = 0, le = externalpoints.size(); i < le; i++)
	{
		if (i == 0)
		{
			if (externalpoints[i].y > 0)
			{
				tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
			}
			else
			{
				tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, 0));
				//tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
			}
		}
		else if (i == le - 1)
		{
			if (externalpoints[i].y > 0)
			{
				tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
			}
			else
			{
				tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, 0));
				//tmppoints.push_back(Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
			}
		}
		else
		{
			tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
		}
	}

	int lowestidx = 0;
	int maxy = 0;
	for (int i = 0, le = tmppoints.size(); i < le; i++)
	{
		if (tmppoints[i].y > maxy)
		{
			maxy = tmppoints[i].y;
			lowestidx = i;
		}
	}

	for (int i = 0; i < lowestidx; i++)
	{
		int starty = tmppoints[i].y;
		int endy = tmppoints[i + 1].y;

		if (starty > endy)
		{
			starty = tmppoints[i + 1].y;
			endy = tmppoints[i].y;
		}
		for (int y = starty; y <= endy; y++)
		{
			int endx;

			if (tmppoints[i].y == tmppoints[i + 1].y)
			{
				uchar * ptrroi = roi.ptr<uchar>(y);
				for (int x = 0; x <= tmppoints[i].x; x++)
				{
					ptrroi[x] = 0;
				}
				continue;
			}

			endx = (tmppoints[i].x - tmppoints[i + 1].x)*(y - tmppoints[i].y) / ((tmppoints[i].y - tmppoints[i + 1].y)) + tmppoints[i].x;

			if (endx > roi.cols - 1)
			{
				endx = roi.cols - 1;
			}

			if (endx < 0)
			{
				endx = 0;
			}

			uchar * ptrroi = roi.ptr<uchar>(y);
			for (int x = 0; x <= endx; x++)
			{
				ptrroi[x] = 0;
			}
		}
	}

	for (int i = lowestidx, le = tmppoints.size(); i < le - 1; i++)
	{
		int starty = tmppoints[i].y;
		int endy = tmppoints[i + 1].y;

		if (starty > endy)
		{
			starty = tmppoints[i + 1].y;
			endy = tmppoints[i].y;
		}
		for (int y = starty; y <= endy; y++)
		{
			int startx;

			if (tmppoints[i].y == tmppoints[i + 1].y)
			{
				uchar * ptrroi = roi.ptr<uchar>(y);
				for (int x = tmppoints[i + 1].x; x<roi.cols; x++)
				{
					ptrroi[x] = 0;
				}
				continue;
			}

			startx = (tmppoints[i].x - tmppoints[i + 1].x)*(y - tmppoints[i].y) / ((tmppoints[i].y - tmppoints[i + 1].y)) + tmppoints[i].x;

			if (startx > roi.cols - 1)
			{
				startx = roi.cols - 1;
			}

			if (startx < 0)
			{
				startx = 0;
			}

			uchar * ptrroi = roi.ptr<uchar>(y);
			for (int x = startx; x <roi.cols; x++)
			{
				ptrroi[x] = 0;
			}
		}
	}

	for (int i = maxy; i < roi.rows; i++)
	{
		memset(roi.ptr<uchar>(i), 0, roi.cols);
	}

	/*tmppoints.clear();


	top = roi.rows - 1;
	for (int i = 0, le = tmppoints.size(); i < le; i++)
	{
	if (top > tmppoints[i].y)
	{
	top = tmppoints[i].y;
	}
	}*/

	tmppoints.clear();
	for (int i = 0, le = externalpoints.size(); i < le; i++)
	{
		tmppoints.push_back(cv::Point(externalpoints[i].x >> 1, externalpoints[i].y >> 1));
	}

	cv::RotatedRect rorect = fitEllipse(tmppoints);

	circ[0] = rorect.center.x;
	circ[1] = rorect.center.y;

	circ[2] = (rorect.size.width + rorect.size.height) / 4;

	for (int i = 0, le = innerpoints.size(); i < le; i++)
	{
		tmppoints.push_back(cv::Point(innerpoints[i].x >> 1, innerpoints[i].y >> 1));
	}

	int right, bottom;

	left = roi.cols - 1; right = 0; top = roi.rows - 1; bottom = 0;
	for (int i = 0, le = tmppoints.size(); i < le; i++)
	{
		if (top > tmppoints[i].y)
		{
			top = tmppoints[i].y;
		}
		if (bottom< tmppoints[i].y)
		{
			bottom = tmppoints[i].y;
		}
		if (left > tmppoints[i].x)
		{
			left = tmppoints[i].x;
		}
		if (right< tmppoints[i].x)
		{
			right = tmppoints[i].x;
		}
	}
	if (left < 0)
	{
		left = 0;
	}
	if (right > gray.cols - 1)
	{
		right = gray.cols - 1;
	}
	if (top < 0)
	{
		top = 0;
	}
	if (bottom > gray.rows - 1)
	{
		bottom = gray.rows - 1;
	}
	if (bottom < top + 10 && right < left + 10)
	{
		isvalid = 0;
		//return isvalid;
	}

	resize(roi, roi_InnerBoundary, cv::Size(srcimg.cols, srcimg.rows), 0, 0, CV_INTER_NN);

	roi = roi(cv::Range(top, bottom), cv::Range(left, right)).clone();
	ROI_gray = gray(cv::Range(top, bottom), cv::Range(left, right)).clone();

	for (int i = 0; i < 10; i++)
	{
		memset(roi.ptr<uchar>(i), 0, roi.step);
		memset(roi.ptr<uchar>(roi.rows - i - 1), 0, roi.step);

	}
	for (int i = 0; i < roi.rows; i++)
	{
		uchar *p = roi.ptr<uchar>(i);
		memset(p, 0, 10);
		memset(p + roi.cols - 10, 0, 10);
	}

	cutx = left * 2;

	bottom = bottom * 2 + 36;
	if (bottom > srcimg.rows)
	{
		bottom = srcimg.rows;
	}
	ROI_img = srcimg(cv::Range(0, bottom), cv::Range(cutx, right * 2));

	cv::Rect rect_cut;
	rect_cut.x = cutx;
	rect_cut.y = 0;
	rect_cut.width = right * 2 - cutx;
	rect_cut.height = bottom;

	return rect_cut;
}

#endif

std::pair<cv::Mat, xmlReadWrite> img_Resize_det(cv::Mat input_img, xmlReadWrite input_xml, int height, int width)
{
	// 存储结果
	cv::Mat output_img;
	xmlReadWrite output_xml;
	std::pair<cv::Mat, xmlReadWrite> output;


	cv::resize(input_img, output_img, cv::Size(width, height), 0, 0, cv::INTER_AREA);

	// 图像质量
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	std::string dst_img_roi = R"(L:\Facevisa_DtylongjunmpbottomDetector_data\111\dst_2\2\resize\)";
	cv::imwrite((dst_img_roi + std::to_string(33) + ".bmp").c_str(), output_img, compression_params);


	output_xml.img_depth = input_xml.img_depth;
	output_xml.img_score = input_xml.img_score;
	output_xml.img_width = width;
	output_xml.img_hight = height;


	float factor_value_x = width*1.0 / input_xml.img_width;
	float factor_value_y = height*1.0 / input_xml.img_hight;



	for (auto idx_obj = 0; idx_obj < input_xml.xml_object_vec.size(); idx_obj++)
	{
		xmlobject newobj;
		newobj.box_name = input_xml.xml_object_vec[idx_obj].box_name;
		newobj.gt_level = input_xml.xml_object_vec[idx_obj].gt_level;
		newobj.gt_boxes.x = input_xml.xml_object_vec[idx_obj].gt_boxes.x*factor_value_x > 0 ? input_xml.xml_object_vec[idx_obj].gt_boxes.x*factor_value_x : 1;
		newobj.gt_boxes.y = input_xml.xml_object_vec[idx_obj].gt_boxes.y*factor_value_y > 0 ? input_xml.xml_object_vec[idx_obj].gt_boxes.y*factor_value_y : 1;
		newobj.gt_boxes.width = int(input_xml.xml_object_vec[idx_obj].gt_boxes.width*factor_value_x + newobj.gt_boxes.x) > width ? int(width - 1 - newobj.gt_boxes.x) : input_xml.xml_object_vec[idx_obj].gt_boxes.width*factor_value_x;
		newobj.gt_boxes.height = int(input_xml.xml_object_vec[idx_obj].gt_boxes.height*factor_value_y + newobj.gt_boxes.y)> height ? int(height - 1 - newobj.gt_boxes.y) : input_xml.xml_object_vec[idx_obj].gt_boxes.height*factor_value_y;
		/*for (auto idx_shape = 0; idx_shape < input_xml.xml_object_vec[idx_obj].gt_shapes.size(); idx_shape++)
		{
		cv::Point point;
		point.x = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].x*factor_value_x;
		point.y = input_xml.xml_object_vec[idx_obj].gt_shapes[idx_shape].y*factor_value_y;
		newobj.gt_shapes.push_back(point);
		}*/
		output_xml.xml_object_vec.push_back(newobj);
	}

	output.first = output_img;
	output.second = output_xml;
	return output;
}

std::vector<std::string> split_str(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;
	size_t size = str.size();
	for (size_t i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

#if SDK_TEST
int main(int argc, char** argv)
{

#if PREDICTION
	std::string src_img;
	std::string dst_img;

	if (argc > 2)
	{
		src_img = argv[1];
		dst_img = argv[2];       //打包可执行文件
	}
	else
	{
#ifdef WIN32
		//src_img = R"(L:\Facevisa_DtylongjunmpbottomDetector_data\20220511_lou\)";
		src_img = R"(J:\顶部边缘相机绊丝标注数据\20231121_imgxml\img\)";
		dst_img = R"(J:\顶部边缘相机绊丝标注数据\20231121_imgxml\res\)";
#else
		src_img = R"(/home1/Facevisa_linux/Windows_Linux/Facevisa_DtyLongjumpBottomDetector_rotate/run/test_rlt/src/)";
		dst_img = R"(/home1/Facevisa_linux/Windows_Linux/Facevisa_DtyLongjumpBottomDetector_rotate/run/test_rlt/dst/)";

#endif
	}

	createFolder(dst_img);
	std::vector<std::string> testImgNames, testImgNamesFull;


#ifdef WIN32

	readFilesRecurse(src_img, testImgNames, testImgNamesFull, 0, ".bmp");
	readFilesRecurse(src_img, testImgNames, testImgNamesFull, 0, ".jpg");
	readFilesRecurse(src_img, testImgNames, testImgNamesFull, 0, ".png");
	readFilesRecurse(src_img, testImgNames, testImgNamesFull, 0, ".jpeg");

#else

	int singnal = ReadImagePath_(src_img, testImgNames);

#endif

	// 加载模型，创建前向engine
	Facevisa_DtyLongjumpTopBottom_Create(&handle, 0);

	void* landmarkhandlepoints_inner_bottom;    //底部内圈
	void* landmarkhandlepoints_outer_bottom;	//底部外圈
	void* landmarkhandlepoints_inner_top;       //顶部内圈
	void* landmarkhandlepoints_outer_top;	    //顶部外圈
	void* landmarkhandlepoints_inner_topside;   //顶部边缘纸管
	void* landmarkhandlepoints_outer_topside;	//顶部边缘丝面
	void* landmarkhandlepoints_bottomside;      //底部边缘相机

	char moduleFileName[MAX_PATH];
#ifdef WIN32

	GetModuleFileNameA(0, moduleFileName, MAX_PATH);
	char* ptr = strrchr(moduleFileName, '\\');
	std::strcpy(++ptr, "templates\\");

#else

	int n = (int)readlink("/proc/self/exe", moduleFileName, 1023/*sizeof(moduleFileName) - 1*/);
	if (n < 0)
	{
		printf("error: %d", n);
	}

	char* ptr = strrchr(moduleFileName, '/');

	strcpy(++ptr, "templates/");

#endif 

	// 图片质量
	std::vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	LongjumpTopBottomPara inputparameter;
	inputparameter.det_thresh = 0.1;   //0.15  0.1
	inputparameter.cas_thresh = 0.5;    //0.65
	inputparameter.longjump_thresh = 100;  //350
	inputparameter.crossjump_thresh = 50;  //50
	inputparameter.edge_thresh = 50;  //55
	inputparameter.papertube_longjumpthresh = 0;  //50
	inputparameter.papertube_crossjumpthresh = 0;  //50
	inputparameter.longJumpDistanceFromTube = 100;//长绊丝距离纸管的距离
	inputparameter.LongjumpGradeLength = 50;//长绊丝分级长度阈值
	inputparameter.bottomside_kp82 = 1;   //底部边缘相机82关键点开关

	std::string sPath = std::string(moduleFileName);
	
	//底部
	std::string Lusterclsmodelpath1 = sPath + "KPDTYBottombrightBobbinOuter.dat";
	std::string Lusterclsmodelpath2 = sPath + "KPDTYBottombrightSilkOuter.dat";
	landmarkhandlepoints_inner_bottom = KPDTYBottombright_Create(Lusterclsmodelpath1);
	landmarkhandlepoints_outer_bottom = KPDTYBottombright_Create(Lusterclsmodelpath2);
	
	//顶部
	std::string Lusterclsmodelpath3 = sPath + "KPDTYTopbrightBobbinInnerOuter.dat";
	std::string Lusterclsmodelpath4 = sPath + "KPDTYTopbrightSilkOuter.dat";
	landmarkhandlepoints_inner_top = KPDTYTopbright_Create(Lusterclsmodelpath3);
	landmarkhandlepoints_outer_top = KPDTYTopbright_Create(Lusterclsmodelpath4);
	//顶部边缘
	std::string Lusterclsmodelpath5 = sPath + "KPDTYSidetopborderbrightBobbinOuter.dat";
	std::string Lusterclsmodelpath6 = sPath + "KPDTYSidetopborderbrightSilkOuter50p.dat";
	landmarkhandlepoints_inner_topside = KPDTYSidetopborderbright_Create(Lusterclsmodelpath5);
	landmarkhandlepoints_outer_topside = KPDTYSidetopborderbright_Create(Lusterclsmodelpath6);
	//底部边缘
	if (inputparameter.bottomside_kp82)//适配新版底部边缘成像
	{
		std::string Lusterclsmodelpath7 = sPath + "KPDTYSidebottomborderbrightBobbinOuterSilkOuterLookdown.dat";
		landmarkhandlepoints_bottomside = KPDTYSidebottomborderbright_Create(Lusterclsmodelpath7);
	}
	else
	{
		std::string Lusterclsmodelpath7 = sPath + "KPDTYSidebottomborderbrightBobbinOuterSilkOuter.dat";
		landmarkhandlepoints_bottomside = KPDTYSidebottomborderbright_Create(Lusterclsmodelpath7);
	}

	int count = 0;
	for (int i = 0; i < testImgNames.size(); ++i)
	{
		if (count % 1 == 0)
		{
			printf("\r [%d/%d]", count + 1, testImgNames.size());
		}
		count++;
		std::string img_name = testImgNames[i];
		std::cout << "total:" << testImgNames.size() << "idx:" << i << "" << img_name << std::endl;

#ifdef WIN32
		std::string img_ori = testImgNamesFull[i];
		if (_access(img_ori.c_str(), 0) == -1)
		{
			continue;
		}
#else
		std::string img_ori = src_img + img_name;
		if (access(img_ori.c_str(), 0) == -1)
		{
			continue;
		}
#endif 
		std::vector<std::string> name_split = split_str(img_name, "_");
		int camera_ID = atoi(name_split[2].c_str());
		inputparameter.camera_ID = camera_ID;

		bool top_flag = false;
		bool bottom_flag = false;
		bool topside_flag = false;
		bool bottomside_flag = false;

		//top
		if (inputparameter.camera_ID == 10)
		{
			top_flag = true;

		}
		if (inputparameter.camera_ID == 1 || inputparameter.camera_ID == 2 || inputparameter.camera_ID == 3 || inputparameter.camera_ID == 4)
		{
			bottom_flag = true;
		}
		//topside
		if (inputparameter.camera_ID >= 33 && inputparameter.camera_ID <= 37)
		{
			topside_flag = true;
		}
		//bottomside
		if (inputparameter.camera_ID >= 38 && inputparameter.camera_ID <= 40)
		{
			bottomside_flag = true;
		}

		cv::Mat image = cv::imread(img_ori, 1);
		if (0 == image.cols || 0 == image.rows || image.empty())
		{
			continue;
		}
		// 图像拷贝
		int im_w = image.cols;
		int im_h = image.rows;
		int im_c = image.channels();
		cv::Mat resultImg = image.clone();

		// 关键点结果
		std::vector<cv::Point> innerpoints_bottom, innerpoints_top, innerpoints_topside;
		std::vector<cv::Point> externalpoints_bottom, externalpoints_top, externalpoints_topside;
		std::vector<cv::Point> inner_externalpoints_bottomside;

		// 颜色转换
		cv::Mat img_gray;
		cv::cvtColor(image, img_gray, CV_BGR2GRAY);
		//bottom
		if (bottom_flag)
		{
			KPDTYBottombright_Inference(landmarkhandlepoints_inner_bottom, img_gray, innerpoints_bottom);
			KPDTYBottombright_Inference(landmarkhandlepoints_outer_bottom, img_gray, externalpoints_bottom);
		}
		
		//top
		if (top_flag)
		{
			KPDTYTopbright_Inference(landmarkhandlepoints_inner_top, img_gray, innerpoints_top);
			KPDTYTopbright_Inference(landmarkhandlepoints_outer_top, img_gray, externalpoints_top);
		}
		
		//topside
		if (topside_flag)
		{
			KPDTYSidetopborderbright_Inference(landmarkhandlepoints_inner_topside, image, innerpoints_topside);
			KPDTYSidetopborderbright_Inference(landmarkhandlepoints_outer_topside, image, externalpoints_topside);
		}
		
		//bottomside
		if (bottomside_flag)
		{
			KPDTYSidebottomborderbright_Inference(landmarkhandlepoints_bottomside, image, inner_externalpoints_bottomside);
		}
		
		// 关键点边界

		Key_Points toplandmarks;                             // 顶部关键点
		Key_Points bottomlandmarks;                          // 底部关键点
		Key_Points topsidelandmarks;                         // 顶部边缘关键点
		Key_Points bottomsidelandmarks;                      // 底部边缘关键点

		bottomlandmarks.innerpoints = innerpoints_bottom;
		bottomlandmarks.outerpoints = externalpoints_bottom;

		toplandmarks.innerpoints = innerpoints_top;
		toplandmarks.outerpoints = externalpoints_top;

		topsidelandmarks.innerpoints = innerpoints_topside;
		topsidelandmarks.outerpoints = externalpoints_topside;

		bottomsidelandmarks.innerpoints = inner_externalpoints_bottomside;
		bottomsidelandmarks.outerpoints = inner_externalpoints_bottomside;

		inputparameter.toplandmarks = toplandmarks;
		inputparameter.bottomlandmarks = bottomlandmarks;
		inputparameter.topsidelandmarks = topsidelandmarks;
		inputparameter.bottomsidelandmarks = bottomsidelandmarks;

#if Show_landmark

		cv::Mat markkpimg;
		image.copyTo(markkpimg);
		for (int i = 0; i < toplandmarks.innerpoints.size(); i++) {
			cv::Point point = toplandmarks.innerpoints[i];
			circle(markkpimg, point, 3, cv::Scalar(0, 0, 255), -1, 8, 0);
			cv::putText(markkpimg, std::to_string(i), cv::Point(point.x, point.y), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 0), 1);
			cv::putText(markkpimg, std::to_string(point.x), cv::Point(point.x, point.y - 25), 1, 1, cv::Scalar(255, 0, 0), 1);
			cv::putText(markkpimg, std::to_string(point.y), cv::Point(point.x, point.y - 50), 1, 1, cv::Scalar(0, 0, 255), 1);
		}

		for (int i = 0; i < toplandmarks.outerpoints.size(); i++) {
			cv::Point point = toplandmarks.outerpoints[i];
			circle(markkpimg, point, 3, cv::Scalar(0, 0, 255), -1, 8, 0);
			cv::putText(markkpimg, std::to_string(i), cv::Point(point.x, point.y), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 0), 1);
			cv::putText(markkpimg, std::to_string(point.x), cv::Point(point.x, point.y - 25), 1, 1, cv::Scalar(255, 0, 0), 1);
			cv::putText(markkpimg, std::to_string(point.y), cv::Point(point.x, point.y - 50), 1, 1, cv::Scalar(0, 0, 255), 1);
		}

		cv::resize(markkpimg, markkpimg, cv::Size(0.5*markkpimg.cols, 0.5*markkpimg.rows));
		cv::imshow("top", markkpimg);
		cv::waitKey(0);

#endif 

		// 检测结果
		Results_LongjumpTopBottom results;
		double start = static_cast<double>(cv::getTickCount());
		Facevisa_DtyLongjumpTopBottom_Inference(handle, image, inputparameter, results);
		double time = ((double)cv::getTickCount() - start) / cv::getTickFrequency();
		std::cout << "time is " << time << std::endl;
		// //显示框和得分
		//for (int index_det = 0; index_det < results.det_boxes.size(); index_det++)
		//{
		//	//检测框的显示,不含背景
		//	
		//	// 显示 box_pre [红色]
		//	cv::rectangle(resultImg, results.det_boxes[index_det], cv::Scalar(0, 0, 255), 2);
		//	// 显示检测得分 [红色]
		//	stringstream os;
		//	os << results.det_score[index_det];    //bboxes[i][5]存放label
		//	string characters = os.str();
		//	cv::putText(resultImg, characters, cv::Point(results.det_boxes[index_det].x, results.det_boxes[index_det].y+50), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255));
		//	cv::putText(resultImg, std::to_string(int(results.det_level[index_det])), cv::Point(results.det_boxes[index_det].x, results.det_boxes[index_det].y), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
		//	
		//}

		bool value_flag = false;

		for (int idx_cls = 0; idx_cls < results.cls_longjump.size(); idx_cls++)
		{
			//results.cls_longjump[idx_cls].longjump_rect.x += cutx;
			cv::putText(resultImg, "longjump_num:", cv::Point(5, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
			cv::putText(resultImg, std::to_string(results.num_of_Longjump), cv::Point(400, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
			cv::putText(resultImg, "crossjump_num:", cv::Point(5, 80), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
			cv::putText(resultImg, std::to_string(results.num_of_crossjump), cv::Point(400, 80), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
			if (results.cls_longjump[idx_cls].longjump_level == 1)
			{
				//cv::rectangle(resultImg, results.cls_longjump[idx_cls].longjump_rect, cv::Scalar(0, 0, 255), 2);				//绘制外接矩形
				cv::drawContours(resultImg, results.cls_longjump[idx_cls].rota_cls_box, -1, { 0, 255, 0 }, 2);				    //绘制旋转框
				stringstream os1;
				os1 << results.cls_longjump[idx_cls].longjump_score;
				string characters1 = os1.str();
				cv::putText(resultImg, characters1, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + 0.4*results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + results.cls_longjump[idx_cls].longjump_rect.height+5), CV_FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));

				// 显示拌丝长度 [红]
				stringstream os2;
				os2 << results.cls_longjump[idx_cls].length;
				string characters2 = os2.str();
				cv::putText(resultImg, characters2, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + 0.8*results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + 0.8*results.cls_longjump[idx_cls].longjump_rect.height+5), CV_FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));
			}
			else if (results.cls_longjump[idx_cls].longjump_level == 2)
			{
				//cv::rectangle(resultImg, results.cls_longjump[idx_cls].longjump_rect, cv::Scalar(255, 0, 0), 2);			//绘制外接矩形
				cv::drawContours(resultImg, results.cls_longjump[idx_cls].rota_cls_box, -1, { 255, 0, 0 }, 2);				//绘制旋转框
				stringstream os1;
				os1 << results.cls_longjump[idx_cls].longjump_score;
				string characters1 = os1.str();

				cv::putText(resultImg, characters1, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + 0.4*results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + results.cls_longjump[idx_cls].longjump_rect.height), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0));

				// 显示拌丝长度 [蓝色]
				stringstream os2;
				os2 << results.cls_longjump[idx_cls].length;
				string characters2 = os2.str();

				cv::putText(resultImg, characters2, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + 0.8*results.cls_longjump[idx_cls].longjump_rect.height), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0));
			}
			else
			{
				//背景类
				cv::Point2f box_left(results.cls_longjump[idx_cls].longjump_rect.x, results.cls_longjump[idx_cls].longjump_rect.y);
				std::vector<cv::Point> landmarks;
				if (top_flag)
				{
					for (int i = 0; i < innerpoints_top.size(); i++)
					{
						landmarks.push_back(innerpoints_top[i]);
					}
					for (int i = 0; i < externalpoints_top.size(); i++)
					{
						landmarks.push_back(externalpoints_top[i]);

					}
				}
				if (bottom_flag)
				{
					for (int i = 0; i < innerpoints_bottom.size(); i++)
					{
						landmarks.push_back(innerpoints_bottom[i]);
					}
					for (int i = 0; i < externalpoints_bottom.size(); i++)
					{
						landmarks.push_back(externalpoints_bottom[i]);

					}
				}
				if (topside_flag)
				{
					for (int i = 0; i < innerpoints_topside.size(); i++)
					{
						landmarks.push_back(innerpoints_topside[i]);
					}
					for (int i = 0; i < externalpoints_topside.size(); i++)
					{
						landmarks.push_back(externalpoints_topside[i]);

					}
				}
				if (bottomside_flag)
				{
					for (int i = 0; i < inner_externalpoints_bottomside.size(); i++)
					{
						landmarks.push_back(inner_externalpoints_bottomside[i]);
					}
				}
				double flag_value = pointPolygonTest(landmarks, box_left, true);   //小于0不在多边形内

				if (true)    // flag_value < 0
				{
					value_flag = true;
					cv::rectangle(resultImg, results.cls_longjump[idx_cls].longjump_rect, cv::Scalar(0, 255, 0), 2);				//绘制外接矩形
					//cv::drawContours(resultImg, results.cls_longjump[idx_cls].rota_cls_box, -1, { 0, 255, 0 }, 2);					//绘制旋转框
																																	// 显示级联得分 [绿色]
					stringstream os1;
					os1 << results.cls_longjump[idx_cls].longjump_score;
					string characters1 = os1.str();
					/*cv::putText(resultImg, characters1, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + 0.4*results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + results.cls_longjump[idx_cls].longjump_rect.height), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));*/

					// 显示拌丝长度 [绿色]
					stringstream os2;
					os2 << results.cls_longjump[idx_cls].length;
					string characters2 = os2.str();
					/*cv::putText(resultImg, characters2, cv::Point(results.cls_longjump[idx_cls].longjump_rect.x + 0.8*results.cls_longjump[idx_cls].longjump_rect.width,
					results.cls_longjump[idx_cls].longjump_rect.y + 0.8*results.cls_longjump[idx_cls].longjump_rect.height), CV_FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255, 0));*/

				}

			}
		}
		// write xml
		if (flag_save_xml_img_det && results.rota_det_boxes.size())
		{
			std::cout << "size:" << results.rota_det_boxes.size() << std::endl;
			xmlReadWrite img_info;
			bool level_flag=false;
			for (auto idx_pos = 0; idx_pos < results.rota_det_boxes.size(); idx_pos++)
			{
				
				/*if (results.rota_det_level[idx_pos] != 0)
				{*/
					level_flag = true;
					img_info.img_width = im_w;
					img_info.img_hight = im_h;
					img_info.img_depth = im_c;


					xmlobject box_info;

					if (results.rota_det_level[idx_pos] == 0)
					{
						box_info.box_name = "longjump";
					}
					else if (results.rota_det_level[idx_pos] == 1)
					{
						box_info.box_name = "crossjump";

					}
					/*else
					{
						box_info.box_name = "__background__";

					}*/
					//if (results.rota_det_level[idx_pos] == 2)
					//{
					//	box_info.box_name = "crossjump";
					//}
					//else
					//{
					//	box_info.box_name = "longjump";
					//}

					box_info.gt_shapes = results.rota_det_boxes[idx_pos];
					img_info.xml_object_vec.push_back(box_info);
				/*}*/

			}
			if (img_info.xml_object_vec.size() && level_flag)
			{

				if (bottom_flag)
				{
					string xml_filetmp = dst_img  + "bottom/";
					createFolder(xml_filetmp);
					string xml_file = xml_filetmp + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp + img_name).c_str(), 0);
				}
				if (top_flag)
				{
					string xml_filetmp1 = dst_img + "top/";
					createFolder(xml_filetmp1);
					string xml_file = xml_filetmp1 + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp1 + img_name).c_str(), 0);
				}
				if (bottomside_flag)
				{
					string xml_filetmp = dst_img + "bottomside/";
					createFolder(xml_filetmp);
					string xml_file = xml_filetmp + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp + img_name).c_str(), 0);
				}
				if (topside_flag)
				{
					string xml_filetmp1 = dst_img + "topside/";
					createFolder(xml_filetmp1);
					string xml_file = xml_filetmp1 + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp1 + img_name).c_str(), 0);
				}
				
			}
		}

		if (flag_save_xml_img_cls && results.cls_longjump.size())
		{
			xmlReadWrite img_info;
			bool level_flag = false;
			for (auto idx_pos = 0; idx_pos < results.cls_longjump.size(); idx_pos++)
			{

				level_flag = true;
				img_info.img_width = im_w;
				img_info.img_hight = im_h;
				img_info.img_depth = im_c;

				xmlobject box_info;

				if (results.cls_longjump[idx_pos].longjump_level == 1)
				{
					box_info.box_name = "longjump";  //longjump
				}
				else if (results.cls_longjump[idx_pos].longjump_level == 2)
				{
					box_info.box_name = "crossjump";   //crossjump

				}
				else
				{
					box_info.box_name = "__background__";

				}
				box_info.gt_boxes = results.cls_longjump[idx_pos].longjump_rect;
				box_info.gt_shapes = results.cls_longjump[idx_pos].rota_cls_box[0];

				img_info.xml_object_vec.push_back(box_info);
			}
			if (img_info.xml_object_vec.size() && level_flag)
			{
				

				if (bottom_flag)
				{
					string xml_filetmp = dst_img + "cls_xml/bottom/";
					createFolder(xml_filetmp);
					string xml_file = xml_filetmp + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp + img_name).c_str(), 0);
				}
				if (top_flag)
				{
					string xml_filetmp1 = dst_img + "cls_xml/top/";
					createFolder(xml_filetmp1);
					string xml_file = xml_filetmp1 + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp1 + img_name).c_str(), 0);
				}
				if (bottomside_flag)
				{
					string xml_filetmp = dst_img + "cls_xml/bottomside/";
					createFolder(xml_filetmp);
					string xml_file = xml_filetmp + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp + img_name).c_str(), 0);
				}
				if (topside_flag)
				{
					string xml_filetmp1 = dst_img + "cls_xml/topside/";
					createFolder(xml_filetmp1);
					string xml_file = xml_filetmp1 + img_name.substr(0, img_name.size() - 4) + ".xml";
					writeXmlFile(xml_file, img_name, img_info);
					CopyFile(img_ori.c_str(), (xml_filetmp1 + img_name).c_str(), 0);
				}
				//string xml_file = dst_img + img_name.substr(0, img_name.size() - 4) + ".xml";
				//writeXmlFile(xml_file, img_name, img_info);
				//CopyFileA(img_ori.c_str(), (dst_img + img_name).c_str(), 0);
				//CopyFile(img_ori.c_str(), (dst_img + img_name).c_str(), 0);
			}
		}

		if (flag_show_detection && results.cls_longjump.size())     //保存分类器结果
		//if (flag_show_detection && results.det_boxes.size())       //保存检测器结果
		{
			// 可视化检测结果
			if (top_flag)
			{
				for (std::vector<Point>::iterator it = innerpoints_top.begin(); it != innerpoints_top.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(255, 0, 0), -1, 8, 0);
				}
				for (std::vector<Point>::iterator it = externalpoints_top.begin(); it != externalpoints_top.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(0, 0, 255), -1, 8, 0);
				}
			}
			if (bottom_flag)
			{
				for (std::vector<Point>::iterator it = innerpoints_bottom.begin(); it != innerpoints_bottom.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(255, 0, 0), -1, 8, 0);
				}
				for (std::vector<Point>::iterator it = externalpoints_bottom.begin(); it != externalpoints_bottom.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(0, 0, 255), -1, 8, 0);
				}
			}
			if (topside_flag)
			{
				for (std::vector<Point>::iterator it = innerpoints_topside.begin(); it != innerpoints_topside.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(255, 0, 0), -1, 8, 0);
				}
				for (std::vector<Point>::iterator it = externalpoints_topside.begin(); it != externalpoints_topside.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(0, 0, 255), -1, 8, 0);
				}
			}
			if (bottomside_flag)
			{
				for (std::vector<Point>::iterator it = inner_externalpoints_bottomside.begin(); it != inner_externalpoints_bottomside.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(255, 0, 0), -1, 8, 0);
				}
				for (std::vector<Point>::iterator it = inner_externalpoints_bottomside.begin(); it != inner_externalpoints_bottomside.end(); it++)
				{
					cv::Point2d point(it->x, it->y);
					cv::circle(resultImg, point, 3, Scalar(0, 0, 255), -1, 8, 0);
				}
			}
			string img_name_new = img_name.substr(0, img_name.size() - 4) + "_mark.png";
			string save_dst = dst_img + "/mark/";
			createFolder(save_dst);
			cv::imwrite((save_dst + img_name_new).c_str(), resultImg, compression_params);
			//	CopyFile(img_ori.c_str(), (dst_img + img_name).c_str(), 0);
		}
#if QA_shujishouji 
		//保存mark图的时候加个for循环筛选图片
		for (int idx_cls = 0; idx_cls < results.cls_longjump.size(); idx_cls++)
		{

			// 可视化检测结果
			for (std::vector<Point>::iterator it = innerpoints.begin(); it != innerpoints.end(); it++)
			{
				cv::Point2d point(it->x, it->y);
				circle(resultImg, point, 3, Scalar(255, 0, 0), -1, 8, 0);
			}

			for (std::vector<Point>::iterator it = externalpoints.begin(); it != externalpoints.end(); it++)
			{
				cv::Point2d point(it->x, it->y);
				circle(resultImg, point, 3, Scalar(0, 0, 255), -1, 8, 0);
			}

			if (results.cls_longjump[idx_cls].longjump_level == 0)
			{
				if (value_flag)
				{
					std::string img_save0 = dst_img + R"(special_normal\)";
					createFolder(img_save0);
					string img_name_new0 = img_name.substr(0, img_name.size() - 4) + "_mark.png";
					cv::imwrite((img_save0 + img_name_new0).c_str(), resultImg, compression_params);
				}
				else
				{

					std::string img_save0 = dst_img + R"(normal\)";
					createFolder(img_save0);
					string img_name_new0 = img_name.substr(0, img_name.size() - 4) + "_mark.png";
					cv::imwrite((img_save0 + img_name_new0).c_str(), resultImg, compression_params);

				}
			}
			else
			{

				std::string img_save1 = dst_img + R"(1\)";
				createFolder(img_save1);
				string img_name_new1 = img_name.substr(0, img_name.size() - 4) + "_mark.png";
				cv::imwrite((img_save1 + img_name_new1).c_str(), resultImg, compression_params);
			}
		}
#endif

	}
	// 释放句柄
	KPDTYBottombright_Release(landmarkhandlepoints_inner_bottom);
	KPDTYBottombright_Release(landmarkhandlepoints_outer_bottom);
	KPDTYTopbright_Release(landmarkhandlepoints_inner_top);
	KPDTYTopbright_Release(landmarkhandlepoints_outer_top);
	KPDTYSidetopborderbright_Release(landmarkhandlepoints_inner_topside);
	KPDTYSidetopborderbright_Release(landmarkhandlepoints_outer_topside);
	KPDTYSidebottomborderbright_Release(landmarkhandlepoints_bottomside);

	Facevisa_DtyLongjumpTopBottom_Release(&handle);
	std::cout << std::endl;
	std::cout << "*-------------------------- 信息汇总 --------------------------*" << std::endl;

#ifdef WIN32
	cv::waitKey(0);
	std::system("pause");

#endif //WIN32


#endif //PREDICTION

	return 0;
}
#endif