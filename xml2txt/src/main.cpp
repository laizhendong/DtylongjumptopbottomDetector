#include "GetXmlBoxes.h"
using namespace std;

//  step1:����xml�е�point�ͱ�ǩ  
//  step2:�������Ĺؼ���Ϣд��txt,ʶ������ѳ̶�����Ϊ0


int main(void)
{
	GetPicFromXmlBox myTest;
	myTest.XmlDir(R"(J:\������Ե�����˿��ע����\20231227_imgxml)");
	myTest.TxtSaveDir(R"(J:\������Ե�����˿��ע����\txt\20231227)");
	myTest.BoxLabel({"longjump", "crossjump" });
	//myTest.BoxLabel({ "__background__", "longjump", "crossjump" });
	myTest.AutoRunning();

	getchar();
}
