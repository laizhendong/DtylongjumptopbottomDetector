#pragma once

#include <vector>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/core/matx.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef WIN32

#ifdef FACEVISA_DtyLongjumpTopBottom_DLL_EXPORTS
#define FACEVISA_Longjump_DLL_EXPORTS __declspec(dllexport)
#else
#define FACEVISA_Longjump_DLL_EXPORTS __declspec(dllimport)
#endif // DEBUG

#else

#define FACEVISA_Longjump_DLL_EXPORTS __attribute__ ((visibility("default")))

#endif



typedef void* LongjumpTopBottomHandle;


struct Key_Points
{
	std::vector < cv::Point >  innerpoints;              // 内圈关键点
	std::vector < cv::Point >  outerpoints;              // 外圈关键点
};


struct LongjumpTopBottomPara {
	float det_thresh;                                    // 检测阈值
	float cas_thresh;                                    // 分类阈值
	float longjump_thresh;                               // 普通长绊丝长度阈值
	float crossjump_thresh;                              // 特殊绊丝长度阈值
	float edge_thresh;                                   // 屏蔽边缘绊丝距离阈值：建议值50--60
	float papertube_longjumpthresh;                      // 屏蔽纸管附近长绊丝距离阈值
	float papertube_crossjumpthresh;                     // 屏蔽纸管附近特殊绊丝距离阈值
	//新增长绊丝剔除逻辑参数
	float longJumpDistanceFromTube;                      //长绊丝距离纸管的距离
	float LongjumpGradeLength;                           //长绊丝分级长度阈值
	Key_Points toplandmarks;                             // 顶部关键点
	Key_Points bottomlandmarks;                          // 底部部关键点
	Key_Points topsidelandmarks;                         // 顶部边缘关键点
	Key_Points bottomsidelandmarks;                      // 底部边缘关键点
	bool bottomside_kp82 = false;                        // 底部边缘相机82关键点开关
	int camera_ID;                                       // 相机号
};

struct longjump_info
{
	cv::Rect longjump_rect;                               //框在原图位置信息(外接矩形)
	std::vector<std::vector<cv::Point>> rota_cls_box;	  //检测框在原图的位置(旋转矩形)
	int longjump_level;								      //等级： 0--背景    1--普通长绊丝     2--特殊绊丝          
	float longjump_score;                                 //得分
	float length;                                         //框的对角线长度
	std::string label;                                    //长绊丝的标签，适配集成框架数据结构
};


typedef struct Results_LongjumpTopBottom
{
	std::vector<std::vector<cv::Point>> rota_det_boxes;			//检测框在原图的位置
	std::vector<float> rota_det_score;                          //检测器的得分
	std::vector<float> rota_det_level;                          //检测器的等级
	std::vector< longjump_info > cls_longjump;					//级联分类器的长绊丝结果
	int num_of_Longjump = 0;									//长绊丝丝的数量，按框计算的累加的
	int num_of_crossjump = 0;									//特殊绊丝的数量
	std::vector<int> num_of_Longjump_allcls;			        //各种类型长绊丝丝的数量：0.（<=半径，<=长度）；1.（<=半径，>长度）；2.（>半径，>=长度）；3.（>半径，>长度）
	cv::Point2f  circle_center_inner;                           //内圈关键点拟合圆圆心
	float circle_radius_inner;                                  //内圈关键点拟合圆半径
};


/**
* @brief	初始化函数
* @param pHandle		 句柄
* @param iDeviceId		 GPU ID
*/  //添加返回说明
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Create(LongjumpTopBottomHandle *pHandle, int iDeviceId = 0);

/**
* @brief	检测主函数
* @param pHandle					句柄
* @param Img					    原图
* @param inputparameter		    	输入参数结构体
* @param results			        检测结果输出结构体
*
* @return 返回说明
*/
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Inference(LongjumpTopBottomHandle pHandle, cv::Mat Img, LongjumpTopBottomPara inputparameter, Results_LongjumpTopBottom& results);

/**
* @brief	内存释放函数
* @param pHandle			句柄
*/
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Release(LongjumpTopBottomHandle *pHandle);

