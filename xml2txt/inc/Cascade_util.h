#ifndef _CASCADE_FUNC_H_
#define _CASCADE_FUNC_H_
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp> 
#include <stdio.h>
// Parse xml
#include <tinyxml.h> 

//#ifdef WIN32
#ifdef WIN32
#include <direct.h>
#include <io.h>
#include <Windows.h>
#else
#include <unistd.h>  //readlink
#include <sys/stat.h>//mkdir
#include <sys/io.h>
#endif
// Search file
#include <list>

/************************************************** 相关变量 ******************************************************/

struct xmlobject
{
	std::string box_name;
	std::string box_pose = "Unspecified";
	int box_truncated = 0;
	int box_difficult = 0;
	int box_area = 0;
	int gt_level = 0;
	cv::Rect gt_boxes;
	std::vector<cv::Point> gt_shapes;
};

struct xmlReadWrite
{
	int img_width = 0;
	int img_hight = 0;
	int img_depth = 0;
	int img_score = 0;
	std::vector< struct xmlobject> xml_object_vec;
};

struct augParameter
{
	int width_crop = 112;       // 最终裁剪resize的尺度
	int height_crop = 112;       // 最终裁剪resize的尺度
	int width_pad = 30;        // 对扰动框加pad
	int height_pad = 30;        // 对扰动框加pad
	int num_aug_min = 10;        // 原始box生成最少的box数目
	int num_aug_max = 50;        // 原始box生成最多的box数目
	int ratio_max = 3;         // 新框长宽比不超过原始box的倍数
	float sacle_min = 0.1;       // 原始box缩放最小值
	float scale_max = 5.0;       // 原始box缩放最大值
	float scale_min_r = 0.01;      // 中心点移动最小范围
	float scale_max_r = 0.25;      // 中心点移动最大范围
	float scale_angle = 36;        // 间隔旋转角度
	float value_iou = 0.5;       // iou阈值
	float value_move_x = 0.20;     // 质心和box中心距离占比
	float value_move_y = 0.20;     // 质心和box中心距离占比
	float value_ratio = 3.0;      // 新框自身长宽比
	float value_zoom = 2.5;      // 单边最大放大尺度 
	float value_fuse = 0.25;     // 复合扰动选取的比例 
	int falg_overlap = 0;        // 0: 根据box计算iou; 1:根据shape点计算iou
	int flag_junheng = 0;
	std::string obj_name_choose = "";
};

/************************************************** xml读写 ******************************************************/

void readXmlFile(std::string xml_file, xmlReadWrite &img_info, std::string obj_name_choose = "");
void writeXmlFile(std::string xml_file, std::string img_name, xmlReadWrite &img_info);
bool checkXmlFile(xmlReadWrite img_info);

/************************************************** csv读写 ******************************************************/

void readCsvFile(std::string csv_file, std::vector<std::pair<std::string, xmlReadWrite>> &img_info, std::string obj_name_choose = "");

/************************************************** 文件创建 ******************************************************/

void checkName(std::string& name);
void createFolder(std::string fodler_path);

/************************************************** 文件读取 ******************************************************/

void readFiles(const std::string &path, std::vector<std::string> &files_name, int *files_number);
void readDirs(const std::string &path, std::vector<std::string> &dir_name);
void readFilesRecurse(const std::string &path, std::vector<std::string> &files_name, std::vector<std::string> &files_name_full, int layer, std::string img_ext);

/************************************************** 图像增广 ******************************************************/

// Blur
std::pair<cv::Mat, xmlReadWrite> img_Blur(cv::Mat input_img, xmlReadWrite input_xml);
std::pair<cv::Mat, xmlReadWrite> img_Blur_Kernel(cv::Mat input_img, xmlReadWrite input_xml, int blur_kernel_size = 3);

// Sharpen
std::pair<cv::Mat, xmlReadWrite> img_Sharpen(cv::Mat input_img, xmlReadWrite input_xml);

// Noise
std::pair<cv::Mat, xmlReadWrite> img_GaussianNoise(cv::Mat input_img, xmlReadWrite input_xml);

// Gamma
std::pair<cv::Mat, xmlReadWrite> img_Gamma(cv::Mat input_img, xmlReadWrite input_xml, float gamma_factor_min = 0, float gamma_factor_max = 0, float famma_factor_fix = 0, bool flag_fix = true);

// Log
std::pair<cv::Mat, xmlReadWrite> img_LogAug(cv::Mat input_img, xmlReadWrite input_xml);

// Contrast
std::pair<cv::Mat, xmlReadWrite> img_ContrastAug(cv::Mat input_img, xmlReadWrite input_xml, float alpha = 1.0, float beta = 0.0, bool flag_fix = true);

// illum
cv::Mat illum_Get_Distance(cv::Mat Img_src, cv::Point point);
cv::Mat illum_Get_Guassian_Map(cv::Mat Img_distance, float value_power, float value_scale, int value_width, int value_height);
cv::Mat illum_Get_Linear_Map(cv::Mat Img_distance, float value_power, float value_scale, int value_width, int value_height);
std::pair<cv::Mat, xmlReadWrite> img_illum_lighting(cv::Mat input_img, xmlReadWrite input_xml, float power_min = 90, float power_max = 110, float scale_min = 70, float scale_max = 150, std::string light_type = "gaussian");

// HSV Jitter
std::pair<cv::Mat, xmlReadWrite> img_HsvJitter(cv::Mat input_img, xmlReadWrite input_xml, float hue, float saturation, float exposure);

// Scale Jitter
std::pair<cv::Mat, xmlReadWrite> img_ScaleJitter(cv::Mat input_img, xmlReadWrite input_xml);

// PCA Jitter
std::pair<cv::Mat, xmlReadWrite> img_PCAJitter(cv::Mat input_img, xmlReadWrite input_xml);

// Channel Shift
std::pair<cv::Mat, xmlReadWrite> img_ChannelShift(cv::Mat input_img, xmlReadWrite input_xml);

// Flip
std::pair<cv::Mat, xmlReadWrite> img_Flip(cv::Mat input_img, xmlReadWrite input_xml, int flag_flip = 1);

// Translation
std::pair<cv::Mat, xmlReadWrite> img_Translation(cv::Mat input_img, xmlReadWrite input_xml, int translation_x = 30, int translation_y = 30);

// Zoom
std::pair<cv::Mat, xmlReadWrite> img_Zoom(cv::Mat input_img, xmlReadWrite input_xml, float zoom_factor_min = 0.8, float zoom_factor_max = 1.2);

// Resize
std::pair<cv::Mat, xmlReadWrite> img_Resize(cv::Mat input_img, xmlReadWrite input_xml, float factor_min_x = 1.0, float factor_max_x = 1.0, float factor_min_y = 1.0, float factor_max_y = 1.0);

// Shearing
std::pair<cv::Mat, xmlReadWrite> img_Shearing(cv::Mat input_img, xmlReadWrite input_xml, float shear_x = 0.5, float shear_y = 0.5);

// Crop
std::pair<cv::Mat, xmlReadWrite> img_Crop(cv::Mat input_img, xmlReadWrite input_xml);

// AffineTrans
std::pair<cv::Mat, xmlReadWrite> img_Affine(cv::Mat input_img, xmlReadWrite input_xml, float affine_x = 0.01, float affine_y = 0.01);

// Rotate
std::pair<cv::Mat, xmlReadWrite> img_Rotate(cv::Mat input_img, xmlReadWrite input_xml, int rotate_angle_start = 0, int rotate_angle_end = 45);

std::pair<cv::Mat, xmlReadWrite> img_Patch(cv::Mat input_img, xmlReadWrite input_xml, int thresh_w, int thresh_h);

// PerspectiveTrans
cv::Mat img_Perspective(cv::Mat src, cv::Point2f* scrPoints, cv::Point2f* dstPoints);

/************************************************** 级联扰动 ******************************************************/


std::pair<cv::Mat, xmlReadWrite> imgAug(cv::Mat input_img, xmlReadWrite input_xml, std::string flag_aug);

void cropImg(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save);
void cropImg2(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save);

void cropImgRotate_unaug(std::pair<cv::Mat, xmlReadWrite> img_aug, struct augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, int center_x, int center_y, std::string save_dir, bool flag_img_show, bool flag_img_save, int flag_to_crop);

void increase_backgroundaug(std::pair<cv::Mat, xmlReadWrite> img_aug, xmlReadWrite &dst_xml, std::string img_name, std::string xml_file);

void cropImgOriginal_unaug(std::pair<cv::Mat, xmlReadWrite> img_aug, augParameter aug_parameter, std::vector<std::string> aug_flags, std::string img_name, std::string save_dir, bool flag_img_show, bool flag_img_save, int flag_to_crop);


void cutByRotatingRec(cv::Mat &img_input, cv::Mat &out_blob, float curAngle, const cv::Point2f center_, float radiu_, int  RecWidth, int RecHeight);
double getDistance(cv::Point A, cv::Point B);
double getAngle(cv::Point A, cv::Point B);

void save_img_xml(std::string dst, std::string img_name, std::string flag_aug, std::pair<cv::Mat, xmlReadWrite> aug);

void modify_xml(xmlReadWrite &src_xml, xmlReadWrite &det_xml, std::string img_name, std::string xml_file);

int check_imgxml(xmlReadWrite &det_xml);

#endif
#pragma warning(disable:4996)
