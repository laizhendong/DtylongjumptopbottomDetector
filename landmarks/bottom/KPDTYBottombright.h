

#pragma once

#include <stdio.h>
#include <vector>
#include <string>
#include "opencv2/opencv.hpp"


#ifdef WIN32

    #ifdef KPDTYBottombright_EXPORT
    #define KPDTYBottombright_API __declspec(dllexport)
    #else
    #define KPDTYBottombright_API __declspec(dllimport)
    #endif
#else
    #define KPDTYBottombright_API __attribute__ ((visibility("default")))
#endif

extern "C"
{
	KPDTYBottombright_API void* KPDTYBottombright_Create(std::string model_path);
	KPDTYBottombright_API int KPDTYBottombright_Inference(void*handle, cv::Mat& gray, std::vector<cv::Point> &result);
	KPDTYBottombright_API int KPDTYBottombright_Release(void *handle);
};
