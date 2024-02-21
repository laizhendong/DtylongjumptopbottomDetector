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
	std::vector < cv::Point >  innerpoints;              // ��Ȧ�ؼ���
	std::vector < cv::Point >  outerpoints;              // ��Ȧ�ؼ���
};


struct LongjumpTopBottomPara {
	float det_thresh;                                    // �����ֵ
	float cas_thresh;                                    // ������ֵ
	float longjump_thresh;                               // ��ͨ����˿������ֵ
	float crossjump_thresh;                              // �����˿������ֵ
	float edge_thresh;                                   // ���α�Ե��˿������ֵ������ֵ50--60
	float papertube_longjumpthresh;                      // ����ֽ�ܸ�������˿������ֵ
	float papertube_crossjumpthresh;                     // ����ֽ�ܸ��������˿������ֵ
	//��������˿�޳��߼�����
	float longJumpDistanceFromTube;                      //����˿����ֽ�ܵľ���
	float LongjumpGradeLength;                           //����˿�ּ�������ֵ
	Key_Points toplandmarks;                             // �����ؼ���
	Key_Points bottomlandmarks;                          // �ײ����ؼ���
	Key_Points topsidelandmarks;                         // ������Ե�ؼ���
	Key_Points bottomsidelandmarks;                      // �ײ���Ե�ؼ���
	bool bottomside_kp82 = false;                        // �ײ���Ե���82�ؼ��㿪��
	int camera_ID;                                       // �����
};

struct longjump_info
{
	cv::Rect longjump_rect;                               //����ԭͼλ����Ϣ(��Ӿ���)
	std::vector<std::vector<cv::Point>> rota_cls_box;	  //������ԭͼ��λ��(��ת����)
	int longjump_level;								      //�ȼ��� 0--����    1--��ͨ����˿     2--�����˿          
	float longjump_score;                                 //�÷�
	float length;                                         //��ĶԽ��߳���
	std::string label;                                    //����˿�ı�ǩ�����伯�ɿ�����ݽṹ
};


typedef struct Results_LongjumpTopBottom
{
	std::vector<std::vector<cv::Point>> rota_det_boxes;			//������ԭͼ��λ��
	std::vector<float> rota_det_score;                          //������ĵ÷�
	std::vector<float> rota_det_level;                          //������ĵȼ�
	std::vector< longjump_info > cls_longjump;					//�����������ĳ���˿���
	int num_of_Longjump = 0;									//����˿˿�����������������ۼӵ�
	int num_of_crossjump = 0;									//�����˿������
	std::vector<int> num_of_Longjump_allcls;			        //�������ͳ���˿˿��������0.��<=�뾶��<=���ȣ���1.��<=�뾶��>���ȣ���2.��>�뾶��>=���ȣ���3.��>�뾶��>���ȣ�
	cv::Point2f  circle_center_inner;                           //��Ȧ�ؼ������ԲԲ��
	float circle_radius_inner;                                  //��Ȧ�ؼ������Բ�뾶
};


/**
* @brief	��ʼ������
* @param pHandle		 ���
* @param iDeviceId		 GPU ID
*/  //��ӷ���˵��
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Create(LongjumpTopBottomHandle *pHandle, int iDeviceId = 0);

/**
* @brief	���������
* @param pHandle					���
* @param Img					    ԭͼ
* @param inputparameter		    	��������ṹ��
* @param results			        ���������ṹ��
*
* @return ����˵��
*/
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Inference(LongjumpTopBottomHandle pHandle, cv::Mat Img, LongjumpTopBottomPara inputparameter, Results_LongjumpTopBottom& results);

/**
* @brief	�ڴ��ͷź���
* @param pHandle			���
*/
FACEVISA_Longjump_DLL_EXPORTS int Facevisa_DtyLongjumpTopBottom_Release(LongjumpTopBottomHandle *pHandle);

