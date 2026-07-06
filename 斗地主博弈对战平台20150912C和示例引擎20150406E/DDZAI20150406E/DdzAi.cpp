//本代码由哈尔滨理工大学计算中心梅险老师及其博弈研究组开发维护，更新及下载 www.amansoft.com
//本代码适用于中国大学生计算机博弈大赛斗地主博弈项目2014版交互协议，计算机博弈大赛Q群:114170410
//本代码仅提供交互协议的用法示范和简单AI博弈思路，开发者需自行改进完善代码参赛，计算机博弈赛-斗地主Q群:57597736
//如有意见和建议请与我们尽早联系QQ:9032753,email:meixian@hrbust.edu.cn
//提示：当前头文件为DdzV200.h在早期版本基础上增加了功能模块，改良了数据结构，因此不兼容DdzV100.h的代码模块
//最新修改日期:2015-04-06
//目前尚存在的不足:H08,H09,H1001,H1002暂不提供首发出牌建议,将在后续版本中陆续修正
#define kPlayerName "参赛选手名称"
#define kPlaMax 500
#include "DdzV200.h"
//D01-START计算当前手中余牌估值,预设不拆对牌和连牌，建议进一步自行完善
//最后修订者:谢文&梅险,最后修订时间:15-02-11
double CalCardsValue(int iPlaOnHand[])	
{
	int i;
	double dSum = 100;			//估值
	for(i=0;iPlaOnHand[i]>=0;i++)
	{
		dSum=dSum-5;			//手牌越少越好没多一张牌优势减5分
		if (i >= 1 && iPlaOnHand[i - 1] / 4 == iPlaOnHand[i] / 4)
			dSum = dSum + 2;	//相邻两牌同点加2分
		if (i >= 4 && iPlaOnHand[i - 4] / 4 <=7
			&&iPlaOnHand[i - 4] / 4 + 1 ==iPlaOnHand[i - 3] / 4 
			&&iPlaOnHand[i - 3] / 4 + 1 ==iPlaOnHand[i - 2] / 4
			&&iPlaOnHand[i - 2] / 4 + 1 ==iPlaOnHand[i - 1] / 4 
			&&iPlaOnHand[i - 1] / 4 + 1 ==iPlaOnHand[i] / 4)
			dSum = dSum + 6;	//2以下相邻五牌单顺加6分
	}
	return dSum; 
}
//D01-END

//I02-START计算己方叫牌策略:预设3分或0分，建议进一步自行完善
//最后修订者:梅险,最后修订时间:15-02-12
int CalBid(struct Ddz * pDdz	)	
{
	int i;
	int iMyBid=3;			//叫牌
	for(i=0;i<3;i++)
		if(pDdz->iBid[i]>=3)
			iMyBid=0;
	return iMyBid;
}
//I02-END


//P030601-START计算己方出牌策略
//最后修订者:夏侯有杰&梅险,最后修订时间:15-02-12
void CalPla(struct Ddz * pDdz	)	
{
	int i;
	double dValueNow;
	double dValueMax=-9999;
	int iMax = 0;
	HelpPla(pDdz);				//主要计算推荐出牌pDdz->iPlaArr[],pDdz->iPlaCount
	for(i=0;i<pDdz->iPlaCount;i++)
	{
		HelpTakeOff(pDdz,i);	//假设取走了第i组牌，将剩余的牌放入pDdz->iPlaOnHand[]
		dValueNow = CalCardsValue(pDdz->iPlaOnHand);			//计算余牌估值
		if (dValueNow > dValueMax)
		{
			dValueMax = dValueNow;
			iMax = i;
		}
	}
	for (i = 0;pDdz->iPlaArr[iMax][i] >= 0; i++)
		pDdz->iToTable[i] = pDdz->iPlaArr[iMax][i];
	pDdz->iToTable[i] = -1;
}
//P030601-END

//P00-START主控模块
//最后修订者:梅险,最后修订时间:15-02-08
int	main( )	
{
	struct Ddz tDdz, *pDdz=&tDdz;
	InitTurn(pDdz);			//初始化数据
	while(pDdz->iStatus!=0)
	{
		InputMsg(pDdz);			//输入信息
		AnalyzeMsg(pDdz);		//分析处理信息
		OutputMsg(pDdz);		//输出信息
		CalOthers(pDdz);		//计算其它数据
	}
	return 0;
}
//P00-END