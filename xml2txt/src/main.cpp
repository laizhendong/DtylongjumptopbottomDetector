#include "GetXmlBoxes.h"
using namespace std;

//  step1:解析xml中的point和标签  
//  step2:将解析的关键信息写入txt,识别的困难程度设置为0


int main(void)
{
	GetPicFromXmlBox myTest;
	myTest.XmlDir(R"(J:\顶部边缘相机绊丝标注数据\20231227_imgxml)");
	myTest.TxtSaveDir(R"(J:\顶部边缘相机绊丝标注数据\txt\20231227)");
	myTest.BoxLabel({"longjump", "crossjump" });
	//myTest.BoxLabel({ "__background__", "longjump", "crossjump" });
	myTest.AutoRunning();

	getchar();
}
