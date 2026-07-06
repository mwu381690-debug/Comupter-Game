#pragma once
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <set>
#include <map>
#include <sstream>
#include <cassert>
#include <cstring> // ע��memset��cstring���
#include <algorithm>
#include <fstream>
#include <vector> 
#include <ciso646>
#include <ctime>
#include <math.h>

// ��ƽ̨�ϣ�C++����ʱĬ�ϰ����˿�
#define maximum(a,b)    (((a) > (b)) ? (a) : (b))
#define minimum(a,b)    (((a) < (b)) ? (a) : (b))
#define Max_level 17 //3-10��Ӧ��levelΪ3-10 J-2��ӦΪ11-15 С��16 ����17 

using namespace std;
ofstream dout("debug.txt");
//ostringstream�����������и�ʽ��������������ڽ���������ת��Ϊstring����
//ostringstreamֻ֧��<<������


template<typename T> string toString(const T& t){
    ostringstream oss;  //����һ����ʽ�������
    oss<<t;             //��ֵ����������
    return oss.str();  
}

enum class CardComboType
{
	PASS, // ��
	SINGLE, // ����
	PAIR, // ����
	TRIPLET, // ����
	TRIPLET1, // ����һ
	TRIPLET2, // ������
	STRAIGHT, // ˳��
	STRAIGHT2, // ˫˳
	BOMB, // ը��
	QUADRUPLE2, // �Ĵ�����ֻ��
	QUADRUPLE4, // �Ĵ������ԣ�
	PLANE, // �ɻ�
	PLANE1, // �ɻ���С��
	PLANE2, // �ɻ�������
	SSHUTTLE, // ����ɻ�
	SSHUTTLE2, // ����ɻ���С��
	SSHUTTLE4, // ����ɻ�������
	ROCKET, // ���
	INVALID // �Ƿ�����
};
int cardComboScores[] = {
	0, // ��
	1, // ����
	2, // ����
	4, // ����
	4, // ����һ
	4, // ������
	6, // ˳��
	6, // ˫˳
	10, // ը��
	8, // �Ĵ�����ֻ��
	8, // �Ĵ������ԣ�
	8, // �ɻ�
	8, // �ɻ���С��
	8, // �ɻ�������
	10, // ����ɻ�����Ҫ���У�����Ϊ10�֣�����Ϊ20�֣�
	10, // ����ɻ���С��
	10, // ����ɻ�������
	16, // ���
	0 // �Ƿ�����
};
 
string cardComboStrings[] = {
	"PASS",
	"SINGLE",
	"PAIR",
	"TRIPLET",
	"TRIPLET1",
	"TRIPLET2",
	"STRAIGHT",
	"STRAIGHT2",
	"BOMB",
	"QUADRUPLE2",
	"QUADRUPLE4",
	"PLANE",
	"PLANE1",
	"PLANE2",
	"SSHUTTLE",
	"SSHUTTLE2",
	"SSHUTTLE4",
	"ROCKET",
	"INVALID"
};

// ��0~53��54��������ʾΨһ��һ����
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// ������0~53��54��������ʾΨһ���ƣ�
// ���ﻹ����һ����ű�ʾ�ƵĴ�С�����ܻ�ɫ�����Ա�Ƚϣ������ȼ���Level��
// ��Ӧ��ϵ���£�
// 3 4 5 6 7 8 9 10	J Q K	A	2	С��	����
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
vector<string> levelDisplayVec = {"3", "4","5","6","7","8","9","10","J","Q","K","A","2","j","J"};

using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/*Hansen*/
constexpr int Landlord	= 0;
constexpr int FarmerA	= 1;
constexpr int FarmerB	= 2;
constexpr Level card2level(Card card){
	return card / 4 + card / 53;
}
struct Seq{//ר�����ڼ��˳�Ӷ���ƵĽṹ�� 
	Level level;
	short len;
};
//A01-START���Ͷ���
//����޶���:÷��,����޶�ʱ��:15-02-08
struct Ddz
{
	int  iStatus;			//����״̬-1����,0����,1��ʼ
	char sCommandIn[80];	//ͨ����������
	char sCommandOut[800];	//ͨ���������
	char sDebug[280];		//��������м���� 
	int iOnHand[21];		//�����ƣ�����ֵ��ʼ��Ϊ-1��
	int iOnTable[162][21];	//�Գ������飨����ֵ��ʼ��Ϊ-2��ÿ����һ���ƣ���-1��β��Pass��Ϊ-1
	int iToTable[21];		//Ҫ������
	char sVer[80];			//Э��汾��
	char sName[80];			//����ѡ�ֳƺ�
	char cDir;				//��ҷ�λ���
	char cLandlord;			//������ҷ�λ���
	char cWinner;			//ʤ���߷�λ���
	int iBid[3];			//���ƹ���
	int iBmax;				//��ǰ��������ֵ��{0,1,2,3}
	int iOTmax;				//��ǰ��������
	int iRoundNow;			//��ǰ�ִ�
	int iRoundTotal;		//���ܾ���
	int iTurnNow;			//��ǰ�ִ�
	int iTurnTotal;			//������
	int iLevelUp;			//����ѡ����
	int iScoreMax;			//ת���÷ֽ���
	int iVoid;				//��Ͼ���м������
	int iLef[3];			//���ֵ���
	int iLastPassCount;		//��ǰ��������PASS����ֵ��[0,2],��ֵ2����������ȡ0��һ��PASSȡ1������PASSȡ2��
	int iLastTypeCount;		//��ǰ��������������ֵ��[0,1108],��ֵ0��iLastPassCount=0ʱ����ֵ��=1ʱ����ԭֵ��=2ʱֵΪ0��
	int iLastMainPoint;		//��ǰ�������Ƶ�����ֵ��[0,15],��ֵ-1��iLastPassCount=0ʱ����ֵ����=1ʱ����ԭֵ��=2ʱֵΪ-1��
	int iPlaArr[kPlaMax][21];		//�������ֳ��ƿ��н⼯�������ƽ����Ʊ���������-1���,-2��β��
	int iPlaCount;			//�������ֳ��ƿ��н�������ֵ��[0��kPlaMax-1]��
	int iPlaOnHand[21];		//����ģ����ƺ����Ʊ���
	Ddz(){
		memset(sCommandIn, 0, 80*sizeof(char));
		memset(sCommandOut, 0, 800*sizeof(char));
		memset(sDebug, 0, 280*sizeof(char));
		memset(sVer, 0, 80*sizeof(char));
		memset(sName, 0, 80*sizeof(char));
		iLevelUp = 0;
		iScoreMax = 0;
		iVoid = 0;
		memset(iLef, 0, 3*sizeof(int));
		memset(iPlaArr, 0, sizeof(iPlaArr));
	}
};
//A01-END

//���ֺ�������
int IsType0Pass(int iCs[]);						//B00
int IsType1Rocket(int iCs[]);					//B01
int IsType2Bomb(int iCs[]);						//B02
int	IsType3Single(int iCs[]);					//B03
int	IsType4Double(int iCs[]);					//B04
int	IsType5Three(int iCs[]);					//B05
int IsType6ThreeOne(int iCs[]);					//B0601
int IsType6ThreeDouble(int iCs[]);				//B0602
int IsType7LinkSingle(int iCs[]);				//B07
int IsType8LinkDouble(int iCs[]);				//B08
int	IsType9LinkThree(int iCs[]);				//B09
int	IsType10LinkThreeSingle(int iCs[]);			//B1001
int	IsType10LinkThreeDouble(int	iCs[]);			//B1002
int IsType11FourSingle(int iCs[]);				//B1101
int IsType11FourDouble(int iCs[]);				//B1102
double CalCardsValue(int iPlaOnHand[]);			//D01
int CountCards(int iCards[]);					//I01
int CalBid(struct Ddz * pDdz);					//I02
int	AnalyzeTypeCount(int iCards[]);				//I03
int	AnalyzeMainPoint(int iCards[]);				//I04
void Help0Pass(struct Ddz * pDdz);				//H00
void Help1Rocket(struct Ddz * pDdz);			//H01
void Help2Bomb(struct Ddz * pDdz);				//H02
void Help3Single(struct Ddz * pDdz);			//H03
void Help4Double(struct Ddz * pDdz);			//H04
void Help5Three(struct Ddz * pDdz);				//H05
void Help6ThreeOne(struct Ddz * pDdz);			//H0601
void Help6ThreeDouble(struct Ddz * pDdz);		//H0602
void Help7LinkSingle(struct Ddz * pDdz);		//H07
void Help8LinkDouble(struct Ddz * pDdz);		//H08
void Help9LinkThree(struct Ddz * pDdz);			//H09
void Help10LinkThreeSingle(struct Ddz * pDdz);	//H1001
void Help10LinkThreeDouble(struct Ddz * pDdz);	//H1002
void Help11FourSingle(struct Ddz * pDdz);		//H1101
void Help11FourDouble(struct Ddz * pDdz);		//H1102
void Rocket(struct Ddz * pDdz);					//01
void Bomb(struct Ddz * pDdz);					//02
void Single(struct Ddz * pDdz);					//03
void Double(struct Ddz * pDdz);					//04
void Three(struct Ddz * pDdz);					//05
void ThreeOne(struct Ddz * pDdz);				//0601
void ThreeDouble(struct Ddz * pDdz);			//0602
void LinkSingle(struct Ddz * pDdz);				//07
void LinkDouble(struct Ddz * pDdz);				//08
void LinkThree(struct Ddz * pDdz);				//09
void LinkThreeSingle(struct Ddz * pDdz);		//1001
void LinkThreeDouble(struct Ddz * pDdz);		//1002
void FourSingle(struct Ddz * pDdz);				//1101
void FourDouble(struct Ddz * pDdz);				//1102
void SortById(int iCards[]);					//M01
void SortByMuch(int iCards[]);					//M02
void SortByMain(int iCards[]);					//M03
void InitRound(struct Ddz * pDdz);				//M04	
void AppendCardsToS(int iCards[],char sMsg[]);	//M05
void HelpPla(struct Ddz * pDdz);				//M06
void HelpTakeOff(struct Ddz *  pDdz,int num);	//M07
void InitTurn(struct Ddz * pDdz);				//P01
void InputMsg(struct Ddz * pDdz, string pCmd);				//P02
void AnalyzeMsg(struct Ddz * pDdz);				//P03
void GetDou(struct Ddz * pDdz);					//P0301
void GetInf(struct Ddz * pDdz);					//P0302
void GetDea(struct Ddz * pDdz);					//P0303
void GetBid(struct Ddz * pDdz);					//P0304
void GetLef(struct Ddz * pDdz);					//P0305
void GetPla(struct Ddz * pDdz);					//P0306
void CalPla(struct Ddz * pDdz);					//P030601	
void UpdateMyPla(struct Ddz * pDdz);			//P030602
void UpdateHisPla(struct Ddz * pDdz);			//P030603
void GetGam(struct Ddz * pDdz);					//P0307
void OutputMsg(struct Ddz * pDdz, vector<string>& answer);				//P04
void CalOthers(struct Ddz * pDdz);				//P05

struct CardCombo;
typedef vector<CardCombo> comboDiv; //�������У�����{"AAA4","33","5","6"} 
typedef struct ComboDiv{//ע�⣡�������Ǵ�д������ĸ 
	comboDiv div;
	float Value;//���Ӽ�ֵ �� �ܼ�ֵ������ 
}COMBODIV;
CardCombo HelpPass(struct Ddz * pDdz);
int Maxlevel(int iCards[]);						//�������е�������
int card2level(int x);
string print(CardCombo combo);
string print(vector<CardCombo> divCombos);

//�������е�������
int Maxlevel(int iCards[]){
	int Max=0;
	for (int i=0;iCards[i]>-1;i++){
		if (card2level(iCards[i])>Max) Max=card2level(iCards[i]);
	} 
	return Max;
}
 
//���Ƶı���ת��Ϊ�Ƶĵ��� 
int card2level(int card){
	return card / 4 + card / 53;
}

//�����
void Rocket(struct Ddz * pDdz)
{
	int iCount;
	if(pDdz->iPlaCount+1<kPlaMax)
	{
		iCount=CountCards(pDdz->iOnHand);
		SortById(pDdz->iOnHand);
		if(iCount>=2 && pDdz->iOnHand[iCount-2] + pDdz->iOnHand[iCount-1] == 52+53)
		//�ж��Ƿ��л�� 
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[iCount-2];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[iCount-1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=-1;
			pDdz->iPlaCount++;
		}
	}
}

//��ը��,��ȡ�߲���ж��ܲ��ܴ���ϼҵĲ��� 
void Bomb(struct Ddz * pDdz){
	SortById(pDdz->iOnHand);
	int cnt=CountCards(pDdz->iOnHand); //�����Ƶ������� 
	if (102==pDdz->iLastTypeCount || cnt<4){
		//����С��4�Ż���Ҫѹ�����ǻ����ض�û�п��з��� 
		return;
	}  
	int i;
	//ͨ���Ʊ����������ը��
	if(204==pDdz->iLastTypeCount)
	{
		for (i=3;pDdz->iOnHand[i]>=0;i++){
			if(pDdz->iPlaCount+1<kPlaMax
					&& card2level(pDdz->iOnHand[i])==card2level(pDdz->iOnHand[i-1])
					&& card2level(pDdz->iOnHand[i-1])==card2level(pDdz->iOnHand[i-2])
					&& card2level(pDdz->iOnHand[i-2])==card2level(pDdz->iOnHand[i-3])
					&& card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint) //�����ϼҵ������� 
			{ 
				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i-3];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i-2];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i-1];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			}  
		}
	}
	else{
		for(i=3;pDdz->iOnHand[i]>=0;i++)					
			if(pDdz->iPlaCount+1<kPlaMax
				&& card2level(pDdz->iOnHand[i])==card2level(pDdz->iOnHand[i-1])				
				&& card2level(pDdz->iOnHand[i-1])==card2level(pDdz->iOnHand[i-2])	
				&& card2level(pDdz->iOnHand[i-2])==card2level(pDdz->iOnHand[i-3]))
			{ 
				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i-3];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i-2];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i-1];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			} 
	}
}

//������ 
void Single(struct Ddz * pDdz)
{
	int i;
	for(i=0;pDdz->iOnHand[i]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax 
			&& ( card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint )  )
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=-1;
			pDdz->iPlaCount++;
		}
	}
}

//������ 
void Double(struct Ddz * pDdz)
{
	int i;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+1]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(card2level(pDdz->iOnHand[i+1])))
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=-1;
			pDdz->iPlaCount++;
		}
	}
}

//������
void Three(struct Ddz * pDdz)
{
	int i;
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+1])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+2]))
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
			pDdz->iPlaArr[pDdz->iPlaCount][3]=-1;
			pDdz->iPlaCount++;
		}
	}
} 

//������һ
void ThreeOne(struct Ddz * pDdz)
{
	int i,iTemp,j;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+1])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+2]))
		{
			iTemp = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				//�õ��Ʋ����������е������ҽ�ը���ų�
				if(j == iTemp || j == iTemp+1 || j == iTemp+2
				 || card2level(pDdz->iOnHand[j]) == card2level(pDdz->iOnHand[i]))
				continue;
				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[j];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			}

		}
	}
} 

//��������
void ThreeDouble(struct Ddz * pDdz)
{
	int i,iTemp,j;
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+1])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+2]))
		{
			iTemp = i;
			for(j = 0 ;pDdz->iOnHand[j+1]>=0 ;j++)
			{
				//�������������е����ų�
				if(j >= iTemp-1 && j <= iTemp+2 )
					continue;
					
				if(card2level(pDdz->iOnHand[j]) == card2level(pDdz->iOnHand[j+1]))
				{
					pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
					pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
					pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
					pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[j];
					pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j+1];
					pDdz->iPlaArr[pDdz->iPlaCount][5]=-1;
					pDdz->iPlaCount++;
				}
			}
		}
	}
} 

//����˳
void LinkSingle(struct Ddz * pDdz)
{   int i,j,k,iLength,iTemp,iCount,iBiaoji,iFlag;
	int iCopyCards[21];
	iCount=CountCards(pDdz->iOnHand);
	iLength=pDdz->iLastTypeCount-700; //˳�ӵĳ��� 
	SortById(pDdz->iOnHand);
	for(i=0;i<21;i++) iCopyCards[i]=pDdz->iOnHand[i];
	for(i=0;iCopyCards[i]>=0;i++)
		if (card2level(iCopyCards[i])>MAX_STRAIGHT_LEVEL) iCopyCards[i]=-1; //2,����С����������˳���� 

	for(i=1;iCopyCards[i]>=0;i++)
		if(card2level(iCopyCards[i])==card2level(iCopyCards[i-1])) iCopyCards[i-1]=-1; //ȥ��������ͬ���� 
		
	for(i=0;i<iCount;i++)
		for(j=i+1;j<iCount;j++)
			if(iCopyCards[i]<iCopyCards[j])
			{ 
				iTemp=iCopyCards[i];
				iCopyCards[i]=iCopyCards[j];
				iCopyCards[j]=iTemp;
			}              //�ѱ�ȥ�������Ƶ��������� 
			
	for(i=0;iCopyCards[i]>=0;i++)
		for(j=i+1;iCopyCards[j]>=0;j++)
			if(iCopyCards[i]>iCopyCards[j])
			{ 
				iTemp=iCopyCards[i];
				iCopyCards[i]=iCopyCards[j];
				iCopyCards[j]=iTemp;
			} //��С��������
	

	for(i=0;iCopyCards[i+iLength-1]>=0;i++)
	{
		iBiaoji=0;
		iFlag=0;
		if(card2level(iCopyCards[i])>pDdz->iLastMainPoint)
		{
			for(j=i;j<iLength-1+i;j++)
			{	
				if(card2level(iCopyCards[j])!=card2level(iCopyCards[j+1])-1)
				{	
					iBiaoji=1;
					break;
				}
			}
		}
		else continue;
		
		if(pDdz->iPlaCount+1<kPlaMax
			&& iBiaoji==0)
		{
			for(k=i;k<iLength+i;k++)
			{
				pDdz->iPlaArr[pDdz->iPlaCount][iFlag]=iCopyCards[k];
				iFlag++;
				if(k==iLength-1+i)			   
				pDdz->iPlaArr[pDdz->iPlaCount][iFlag++]=-1;
			}
			pDdz->iPlaCount++;
		}
	}
	
} 

//˫˳
void LinkDouble(struct Ddz * pDdz){
	
	int i,j,k,iLength;
	int iCopyCards[21];
	int n=1;
	int iFlag=0;
	int iCardsFlag=0;
	int iCard=0;
	SortById(pDdz->iOnHand);
	iLength=(pDdz->iLastTypeCount-800)/2; //˫˳�ĳ��ȣ���¼������
	 
	for(i=0;i<21;i++) iCopyCards[i]=pDdz->iOnHand[i];
	for(i=0;iCopyCards[i]>=0;i++)
		if(card2level(iCopyCards[i])>MAX_STRAIGHT_LEVEL) iCopyCards[i]=-1; //2�ʹ���С�������������� 
		
	for(i=0;i<CountCards(iCopyCards);i++)
	{
		if (card2level(iCopyCards[i])!=card2level(iCopyCards[i+1]))
		{
			for(j=i;j<CountCards(iCopyCards);j++)
				iCopyCards[j]=iCopyCards[j+1];
			i--;
		}
	}//���������� ��������� ���ӱ䵥�� ���Ʊ�û��
	
	for(i=0;i<CountCards(iCopyCards);i++)
	{
		if(card2level(iCopyCards[i])==card2level(iCopyCards[i+1]))
		{
			for(j=i;j<CountCards(iCopyCards);j++)
				iCopyCards[j]=iCopyCards[j+1];
			i--;
		}
	}
	//�����Ͷ��ӱ�ɵ��ƣ���������һ���������Ѿ�����˵��� 
	
	//���Ͻ�����iCopyCards[21]����������ɶ��ӵĵ������������ΪA

	for(i=0;iCopyCards[i]>=0;i++)
	{
		if (card2level(iCopyCards[i])==card2level(iCopyCards[i+1])-1) n++;
		else
		{
			//��ʼ�� 
			n=1;
			i=iFlag;
			iFlag++;
		}
		
		if(n==iLength)
		{
			//�ҵ����ϳ���Ҫ���˳��
			if(pDdz->iPlaCount+1<kPlaMax
				&& card2level(iCopyCards[i+2-iLength])>pDdz->iLastMainPoint)
			{
				for(j=i+2-iLength;j<=i+1;j++)
				{
					for (k=0;k<CountCards(pDdz->iOnHand);k++)
					{
						if(card2level(iCopyCards[j])==card2level(pDdz->iOnHand[k])&&iCardsFlag<2)
						{
							//��������Ӽӵ����Ʒ����� 
							pDdz->iPlaArr[pDdz->iPlaCount][iCard]=pDdz->iOnHand[k];
							iCard++;
							iCardsFlag++;
						}
					}
					iCardsFlag=0;
				}
				pDdz->iPlaArr[pDdz->iPlaCount][iCard]=-1;
				pDdz->iPlaCount++;
				iCard=0;		
			}
			
			//��ʼ�� 
			n=1;
			i=iFlag;
			iFlag++;
		}
	}
}  

//����˳�������Ƶģ� 
void LinkThree(struct Ddz * pDdz){
	if(CountCards(pDdz->iOnHand)<6) return; //��������С��6���򲻿����зɻ�
	int iTempArr[15][5]; //iTempArr[i][j] ��ʾlevelΪi������j��
	memset(iTempArr,-1,sizeof(iTempArr)); 
	int i,j,k;
	int iLength = pDdz->iLastTypeCount-900; //��˳�ĳ��� 
	int iTemp;
	 
	SortById(pDdz->iOnHand);    //�Ƚ������������
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48) //2�ʹ���С�����ܵ���˳�� 
		{
			j = 1;
			while(-1 != iTempArr[card2level(pDdz->iOnHand[i])][j]) j++;
			//��Ӧ������j�ŵı��� 
			iTempArr[card2level(pDdz->iOnHand[i])][j] = pDdz->iOnHand[i];
			
			//��Ӧ���������ж����� 
			iTempArr[card2level(pDdz->iOnHand[i])][0] = j;
		}
	}
	
	for(i = pDdz->iLastMainPoint+1 ; i <15 ; i++)
	{
		iTemp = iLength-3;
		if(iTempArr[i][0]>=3)
		{
			j = i;
			while (iTemp)
			{
				if(iTempArr[j][0]>=3)
				{
					iTemp-=3;
					j++; //˳������ĸ�level 
				}
				else break;
			}
			
			//iTemp>0 �򲻹��� 
			if (pDdz->iPlaCount+1<kPlaMax
				&& iTemp == 0)
			{
				k = 0;
				while (k < iLength)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}
				pDdz->iPlaCount++;
			}
		}
	}


}


//����˳���� 
void LinkThreeSingle(struct Ddz * pDdz)
{
	int i,iTemp,j,k,m,iLength = (pDdz->iLastTypeCount-1000);
	int iTempArr[15][5];
	int nFlag = 0;
	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı���
	
	memset(iTempArr,-1,sizeof(iTempArr));
	SortById(pDdz->iOnHand);    //�Ƚ������������
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48)
		{
			j = 1;
			while(-1 != iTempArr[card2level(pDdz->iOnHand[i])][j])j++;

			//���������iTempArr[card2level(pDdz->iOnHand[i])][j]���棬��Ϊ�ñ���ĵ���Ϊcard2level(pDdz->iOnHand[i])
			iTempArr[card2level(pDdz->iOnHand[i])][j] = pDdz->iOnHand[i];

			//������Ϊ card2level(pDdz->iOnHand[i]) ���Ƶ����� ��Ϊ j ��
			iTempArr[card2level(pDdz->iOnHand[i])][0] = j;
		}
	}

	for(i = pDdz->iLastMainPoint+1 ; i <15 ; i++)
	{
		iTemp = iLength; //Ŀ�곤��
		 
		if(iTempArr[i][0]>=3) //��Ӧ�������ƴ��ڵ������� 
		{
			j = i;
			while (iTemp)
			{
				if(iTempArr[j][0]>=3)
				{
					iTemp-=4;
					j++;
				}
				else break;
			} 
			if(iTemp == 0) //�ﵽ��Ŀ�곤�ȣ������ɳ��Ʒ�����Ѱ�Ҵ��� 
			{
				
				k = 0;
				j--;
				while (k < iLength/4*3)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}
				
				//Ѱ�Ҵ��� 
				for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
				{
					nFlag = 1;
					for(m = 0;m < k;m++)
					{
						if(pDdz->iOnHand[j] == pDdz->iPlaArr[pDdz->iPlaCount][m])
						{
							nFlag = 0;
							break;
						}
					}
					
					if(pDdz->iPlaCount+1<kPlaMax
						&& nFlag)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j];
						k++;
						if(k == iLength) //������� 
						{
							pDdz->iPlaCount++;
							break;
						}
					}
				}
			}			
		}
	}
}


//����˳��˫
void LinkThreeDouble(struct Ddz * pDdz)
{
	int i,iTemp,j,k,m,iLength = (pDdz->iLastTypeCount-1000);
	int iTempArr[15][5];
	int nFlag = 0;

	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı���
	
	memset(iTempArr,-1,sizeof(iTempArr));
	SortById(pDdz->iOnHand);    //�Ƚ������������
	
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48)
		{
			j = 1;
			while(-1 != iTempArr[card2level(pDdz->iOnHand[i])][j]) j++;

			//���������iTempArr[card2level(pDdz->iOnHand[i])][j]���棬��Ϊ�ñ���ĵ���Ϊcard2level(pDdz->iOnHand[i])
			iTempArr[card2level(pDdz->iOnHand[i])][j] = pDdz->iOnHand[i];

			//������Ϊ card2level(pDdz->iOnHand[i]) ���Ƶ����� ��Ϊ j ��
			iTempArr[card2level(pDdz->iOnHand[i])][0] = j;
		}
	}

	for(i = pDdz->iLastMainPoint+1 ; i <15 ; i++)
	{
		iTemp = iLength;
		if(iTempArr[i][0]>=3)
		{
			j = i;
			while (iTemp)
			{
				if(iTempArr[j][0]>=3)
				{
					iTemp-=5;
					j++;
				}
				else break;
			}
			
			if(iTemp == 0)
			{
				k = 0;
				j--;
				while (k < iLength/5*3)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}

				for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
				{
					nFlag = 1;
					for(m = 0;m < k;m++)
					{
						if(pDdz->iOnHand[j] == pDdz->iPlaArr[pDdz->iPlaCount][m])
						{
							nFlag = 0;
							break;
						}
					}
					if(pDdz->iPlaCount+1<kPlaMax
						&& nFlag)
					{
						if(card2level(pDdz->iOnHand[j]) != card2level(pDdz->iOnHand[j+1])) continue;
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j];
						k++;
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j+1];
						k++;
						if(k == iLength)
						{
							pDdz->iPlaCount++;
							break;
						}
					}
				}
			}
		}
	}
}

//���Ĵ����� 
void FourSingle(struct Ddz * pDdz)
{
	int i,iTempI,j,k;
	if(CountCards(pDdz->iOnHand)<6) return ;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+3]>=0;i++)
	{
		if(card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+1])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+2])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+3]))
		{
			iTempI = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				//�õ��Ʋ����������е���
				if(j == iTempI || j == iTempI+1 || j == iTempI+2 || j == iTempI+3) continue;
				
				for(k = j+1 ; pDdz->iOnHand[k]>=0 ;k++)
				{
					//�õ��Ʋ����������е���
					if(k == iTempI || k == iTempI+1 || k == iTempI+2 || k == iTempI+3) continue;
					
					if(pDdz->iPlaCount+1<kPlaMax)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
						pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
						pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
						pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i+3];
						pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j];
						pDdz->iPlaArr[pDdz->iPlaCount][5]=pDdz->iOnHand[k];
						pDdz->iPlaArr[pDdz->iPlaCount][6]=-1;
						pDdz->iPlaCount++;
					}
				}
			}
		}
	}
}

//���Ĵ�����
void FourDouble(struct Ddz * pDdz)
{
	int i,iTempI,j,k;
	if(CountCards(pDdz->iOnHand)<8) return ;
	
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+3]>=0;i++)
	{
		if(card2level(pDdz->iOnHand[i]) > pDdz->iLastMainPoint 
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+1])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+2])
			&& card2level(pDdz->iOnHand[i]) == card2level(pDdz->iOnHand[i+3]))
		{
			iTempI = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				if(j == iTempI || j == iTempI+1 || j == iTempI+2 || j == iTempI+3 || card2level(pDdz->iOnHand[j]) != card2level(pDdz->iOnHand[j+1])) continue;
				
				for(k = j+2 ; pDdz->iOnHand[k]>=0 ;k++)
				{
					//�ö��Ӳ����������е����ҰѴ�С���ų���
					if(k == iTempI || k == iTempI+1 || k == iTempI+2 || k == iTempI+3|| card2level(pDdz->iOnHand[k]) != card2level(pDdz->iOnHand[k+1]))
					continue;
					
					if(pDdz->iPlaCount+1<kPlaMax)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
						pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
						pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
						pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i+3];
						pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j];
						pDdz->iPlaArr[pDdz->iPlaCount][5]=pDdz->iOnHand[j+1];
						pDdz->iPlaArr[pDdz->iPlaCount][6]=pDdz->iOnHand[k];
						pDdz->iPlaArr[pDdz->iPlaCount][7]=pDdz->iOnHand[k+1];
						pDdz->iPlaArr[pDdz->iPlaCount][8]=-1;
						pDdz->iPlaCount++;
					}
				}
			}
		}
	}
} 


//B00-START�жϳ��������Ƿ�Ϊ��Ȩ
//����޶���:÷��,����޶�ʱ��:15-02-10
int IsType0Pass(int iCs[])
{
	int iCount;
	iCount = CountCards(iCs);
	if(iCount==0)
		return 1;
	return 0;
}
//B00-END

//B01-START�жϳ��������Ƿ�Ϊ���
//����޶���:�ĺ��н�&÷��,����޶�ʱ��:15-02-10
int IsType1Rocket(int iCs[])
{
	if((iCs[2]==-1) && (iCs[0] + iCs[1]== 52 + 53))
		return 1;
	return 0;
}
//B01-END

//B02-START�жϳ��������Ƿ�Ϊը��
//����޶���:�ĺ��н�&÷��,����޶�ʱ��:15-03-10
//�޶����ݼ�Ŀ��:��ֹ���ǿյ�
int IsType2Bomb(int iCs[])
{
	if(4 != CountCards(iCs))
		return 0;
	if((iCs[4]==-1) && ( iCs[0]/4!= -1 && iCs[0]/4==iCs[1]/4 && iCs[0]/4==iCs[2]/4  && iCs[0]/4==iCs[3]/4 ))
		return 1;
	return 0;
}
//B02-END

//B03-START�жϳ��������Ƿ�Ϊ����
//����޶���:�ĺ��н�,����޶�ʱ��:15-03-10
//�޶����ݼ�Ŀ��:if�ж���������
int	IsType3Single(int iCs[])
{
	if(iCs[0] != -1 && iCs[1] == -1)
		return 1;
	return 0;
}
//B03-END

//B04-START�жϳ��������Ƿ�Ϊ����
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-13
int	IsType4Double(int iCs[])
{
	if(IsType1Rocket(iCs))
		return 0;
	if(iCs[0]/4 == iCs[1]/4 && iCs[0] != -1 && iCs[2] == -1)
		return 1;
	return 0;
}
//B04-END

//B05-�жϳ��������Ƿ�Ϊ����
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-13
int	IsType5Three(int iCs[])
{
	if(iCs[0] != -1 && iCs[0]/4 == iCs[1]/4 && iCs[0]/4 == iCs[2]/4 && iCs[3] == -1)
		return 1;
	return 0;
}
//B05-END

//B0601-START�жϳ��������Ƿ�Ϊ����һ��
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-12
int IsType6ThreeOne(int iCs[])
{
	if(IsType2Bomb(iCs) || 4 != CountCards(iCs))
		return 0;
	SortByMuch(iCs);
	if(iCs[0]/4 == iCs[1]/4 && iCs[0]/4 == iCs[2]/4)
		return 1;
	return 0;
}
//B0601-END

//B0602-START�жϳ��������Ƿ�Ϊ����һ��
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-12
int IsType6ThreeDouble(int iCs[])
{
	if(5 != CountCards(iCs) )
		return 0;
	SortByMuch(iCs);
	if(iCs[0]/4==iCs[1]/4 && iCs[0]/4==iCs[2]/4)
		if(iCs[3]/4 == iCs[4]/4)
			return 1;
	return 0;
}
//B0602-END

//B07-START�жϳ��������Ƿ�Ϊ��˳
//����޶���:л��,����޶�ʱ��:15-02-12
int IsType7LinkSingle(int iCs[])
{
	int iCount;
	int i;
	iCount=CountCards(iCs);
	if(iCount>=5)
	{
		SortById(iCs);
		for(i=1;iCs[i]>=0;i++)
			if(iCs[i-1]/4+1 != iCs[i]/4 || iCs[i]>=48)
				return 0;
		return 1;
	}
	return 0;
}
//B07-END

//B08-START�жϳ��������Ƿ�Ϊ����
//����޶���:л��&�ĺ��н�,����޶�ʱ��:15-03-10
int IsType8LinkDouble(int iCs[])
{   
	int iCount = CountCards(iCs);
	int i; 
	if(iCount < 6 || iCount % 2 != 0)
		return 0;
	SortById(iCs);   //��iCs������
	for(i = 1 ; i < iCount ; i++)
	{
		//�ж�i�ǵ�������˫��
		if(i%2)
		{
			//���i�ǵ��������i���Ƶı������������һ���Ʊ���+1  
			if(iCs[i]/4 != iCs[i-1]/4)
				return 0;
		}
		else
		{
			//���i��˫�������i���Ƶı������������һ���Ʊ���
			if(iCs[i]/4 != iCs[i-1]/4 + 1)
				return 0;
		}
	}
	return 1;
}
//B08-END

//B09-START�жϳ��������Ƿ�Ϊ��˳
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-13
int	IsType9LinkThree(int iCs[])
{
	int iTempArr[11] = {0};   //��ʼ������iTempArr������¼ 3 - A ÿ������������
	int iCount = CountCards(iCs);
	int i, iMinNum , iMaxNum;   //iMinNum Ϊ iCs ��С����,iMaxNum Ϊ iCs ������
	if(iCount < 6 || iCount % 3 != 0)
		return 0;
	SortById(iCs);   //��iCs������
	iMinNum = iCs[0]/4;
	iMaxNum = iMinNum + iCount/3 -1;
	for(i = 0 ; i < iCount ; i++)
	{
		//�ж�iCs[i]�Ƿ�����Ч������Χ��
		if (iCs[i] > 47 || iCs[i]/4 < iMinNum || iCs[i]/4 > iMaxNum) 
		{
			return 0;
		}
		iTempArr[iCs[i]/4]++;
	}

	for (i = iMinNum ;i <= iMaxNum;i++)
	{
		//�ж��Ƿ�ÿ����Ч������Ϊ3����
		if(iTempArr[i] != 3)
			return 0;
	}
	return 1;

}
//B09-END

//B1001-START�ж���˳����������1�ǣ�0����
//����޶���:�ĺ��н�,����޶�ʱ��:15-03-10
//�޶����ݼ�Ŀ��:��ֹ44455556�ĳ���ʱ�����˳����
int	IsType10LinkThreeSingle(int iCs[])
{
	int iCount = CountCards(iCs);
	int iTempArr[18];
	int i,n ,m , j , iNum , iTemp ; //iNum��¼�ж��ٸ� 3+1
	if(iCount < 8 || iCount % 4 != 0)
		return 0;
	memset(iTempArr,-1,18*4);   //��ʼ��iTempArr��ֵ��Ϊ-1
	iNum = iCount/4;  
	SortByMuch(iCs);   //����
	//�ж��ǲ�����ը��
	n = 1,m = 0;
	while (n)
	{
		for(i = m ; i < m+4;i++)
			iTempArr[i] = iCs[i];

		//�ж�iTempArr�ǲ���ը��������������
		if(0 == IsType2Bomb(iTempArr))
		{	
			n = 0;
		}
		else
		{
			//����ǵĻ�,��ô���ը��Ӧ����һ��˳�Ӽ�һ�����ƣ����ž�Ӧ�÷���iCs�ĺ���
			iTemp = iCs[m];
			for(j = m+1 ; j < iCount ;j++)
			{
				iCs[j-1] = iCs[j];
				iCs[j] = iTemp;
				iTemp = iCs[j];
			}
			m = m+3;
		}
		memset(iTempArr,-1,18*4);
	}

	//����˳��ֵ��iTempArr
	for(i = 0 ; i < 3*iNum;i++)
	{
		iTempArr[i] = iCs[i];   
	}

	//�ж�iTempArr�ǲ�����˳
	if(1 == IsType9LinkThree(iTempArr))
	{
		//��iTempArr����iCs   ��ֹ55544465���������ͳ���
		for(i = 0 ; i < 3*iNum;i++)
		{
			iCs[i] = iTempArr[i];
		}
		return 1;
	}
	return 0;

}
//B1001-END


//B1002-START   �ж���˳���ԣ�����1�ǣ�0����
//����޶���:�ĺ��н�,����޶�ʱ��:15-02-13
int	IsType10LinkThreeDouble(int	iCs[])
{
	int iCount = CountCards(iCs);
	int iTempArr[18];
	int i,n , j ,k, iNum , iTemp ; //iNum��¼�ж��ٸ� 3+2 
	if(iCount < 10 || iCount % 5 != 0)
		return 0;
	memset(iTempArr,-1,18*4);   //��ʼ��iTempArr��ֵ��Ϊ-1
	iNum = iCount/5;
	SortByMuch(iCs);   //����
	//�ж��ǲ�����ը��
	n = 1;
	while (n)
	{
		for(i = 0 ; i < 4;i++)
		{
			iTempArr[i] = iCs[i];
		}

		//�ж�iTempArr�ǲ���ը��������������
		if(0 == IsType2Bomb(iTempArr))
		{	
			n = 0;
		}
		else
		{
			//����ǵĻ�,��ô���ը��Ӧ��������2�����ӣ���Ӧ�÷���iCs�ĺ���
			for(k = 0 ; k < 4 ; k ++)
			{
				iTemp = iCs[0];
				for(j = 1 ; j < iCount ;j++)
				{
					iCs[j-1] = iCs[j];
					iCs[j] = iTemp;
					iTemp = iCs[j];
				}
			}
		}
		memset(iTempArr,-1,18*4);
	}

	//����˳��ֵ��iTempArr
	for(i = 0 ; i < 3*iNum;i++)
	{
		iTempArr[i] = iCs[i];   
	}

	//�ж�iTempArr�ǲ�����˳
	if(0 == IsType9LinkThree(iTempArr))
		return 0;

	//����iTempArr
	memset(iTempArr,-1,18*4);

	j = 0;
	for(i = 3*iNum ;i < iCount;i++)
	{

		iTempArr[j] = iCs[i];
		if(j == 1)
		{
			//�ж��ǲ��Ƕ���
			if(0 == IsType4Double(iTempArr))
				return 0;
			memset(iTempArr,-1,18*4);
			j = 0;
		}
		else
		{
			j++;
		}
	}

	return 1;
}
//B1002-END

//B1101-START�жϳ��������Ƿ�Ϊ�Ĵ�����
//����޶���:÷��,����޶�ʱ��:15-02-10
int IsType11FourSingle(int iCs[])
{
	int iCount;
	iCount=CountCards(iCs);
	SortByMuch(iCs);  //ͬ�������Ƶ�����ǰ��
	if(iCount==6)  //6=4+1+1
		if(iCs[0]/4==iCs[1]/4 && iCs[0]/4==iCs[2]/4  && iCs[0]/4==iCs[3]/4 )  //����
			return 1;
	return 0;
}
//B1101-END

//B1102-START�жϳ��������Ƿ�Ϊ�Ĵ�����
//����޶���:÷��,����޶�ʱ��:15-02-10
int IsType11FourDouble(int iCs[])
{
	int iCount;
	iCount=CountCards(iCs);
	SortByMuch(iCs);  //ͬ�������Ƶ�����ǰ��
	if(iCount==8)
		if(iCs[0]/4==iCs[1]/4 && iCs[0]/4==iCs[2]/4  && iCs[0]/4==iCs[3]/4 )  //����
			if(iCs[4]/4==iCs[5]/4 && iCs[6]/4==iCs[7]/4)  //8=4+2+2
				return 1;
	return 0;
}
//B1102-END

//I01-START��������
//����޶���:÷��,����޶�ʱ��:15-02-11
int CountCards(int iCards[])
{
	int iCount=0;
	while(iCards[iCount]>-1)
		iCount++;
	return iCount;
}
//I01-END

//I03-START�������ͺ�����
//����޶���:÷��,����޶�ʱ��:15-03-01
int	AnalyzeTypeCount(int iCards[])	
{
	int iCount=0;
	iCount=CountCards(iCards);
	if(IsType0Pass(iCards))
		return 0*100+iCount;
	if(IsType1Rocket(iCards))
		return 1*100+iCount;
	if(IsType2Bomb(iCards))
		return 2*100+iCount;
	if(IsType3Single(iCards))
		return 3*100+iCount;
	if(IsType4Double(iCards))
		return 4*100+iCount;
	if(IsType5Three(iCards))
		return 5*100+iCount;
	if(IsType6ThreeOne(iCards))
		return 6*100+iCount;
	if(IsType6ThreeDouble(iCards))
		return 6*100+iCount;
	if(IsType7LinkSingle(iCards))
		return 7*100+iCount;
	if(IsType8LinkDouble(iCards))
		return 8*100+iCount;
	if(IsType9LinkThree(iCards))
		return 9*100+iCount;
	if(IsType10LinkThreeSingle(iCards))
		return 10*100+iCount;
	if(IsType10LinkThreeDouble(iCards))
		return 10*100+iCount;
	if(IsType11FourSingle(iCards))
		return 11*100+iCount;
	if(IsType11FourDouble(iCards))
		return 11*100+iCount;

	return -1;
}
//I03-END

//I04-START����������С����
//����޶���:÷��,����޶�ʱ��:15-03-01
int	AnalyzeMainPoint(int iCards[])	
{
	if(IsType0Pass(iCards))
		return -1;
	SortByMain(iCards);
	return iCards[0]/4;
}
//I04-END

//H00-START��iOnHand�в�ѯ��Ȩ���͵�iPlaArr,Ŀǰ˫˳����˳���Ʋ����׳�
//����޶���:÷��,����޶�ʱ��:15-02-17 12:00 
void Help0Pass(struct Ddz * pDdz)
{
		Help1Rocket(pDdz);
		Help2Bomb(pDdz);
		Help3Single(pDdz);
		Help4Double(pDdz);
		Help5Three(pDdz);
		Help6ThreeOne(pDdz);
		Help6ThreeDouble(pDdz);
		Help7LinkSingle(pDdz);
//		Help8LinkDouble(pDdz);
//		Help9LinkThree(pDdz);
//		Help10LinkThreeSingle(pDdz);
//		Help10LinkThreeDouble(pDdz);
		Help11FourSingle(pDdz);
		Help11FourDouble(pDdz);
}
//H00-END

//H01-START��iOnHand�в�ѯ��ϻ�����͵�iPlaArr
//����޶���:����&÷��,����޶�ʱ��:15-02-17 12:00 
void Help1Rocket(struct Ddz * pDdz)
{
	int iCount;
	if(pDdz->iPlaCount+1<kPlaMax)
	{
		iCount=CountCards(pDdz->iOnHand);
		SortById(pDdz->iOnHand);
		if(iCount>=2 && pDdz->iOnHand[iCount-2] + pDdz->iOnHand[iCount-1] == 52+53)
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[iCount-2];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[iCount-1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=-1;
			pDdz->iPlaCount++;
		}
	}
}
//H01-END

//H02-START�Ƽ�����Ӧ��ը��
//����޶���: л�ģ�����޶�ʱ��: 2015-03-10
void Help2Bomb(struct Ddz * pDdz)
{
	int i,iCount;
	SortById(pDdz->iOnHand);
	iCount=CountCards(pDdz->iOnHand);
	if (102==pDdz->iLastTypeCount || iCount<4)
		return;
	if(204==pDdz->iLastTypeCount)
	{
		for(i=3;pDdz->iOnHand[i]>=0;i++)
			if(pDdz->iPlaCount+1<kPlaMax
				&& pDdz->iOnHand[i]/4==pDdz->iOnHand[i-1]/4
				&& pDdz->iOnHand[i-1]/4==pDdz->iOnHand[i-2]/4
				&& pDdz->iOnHand[i-2]/4==pDdz->iOnHand[i-3]/4
				&& pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint)
			{ 
				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i-3];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i-2];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i-1];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			}
	}
	else
		for(i=3;pDdz->iOnHand[i]>=0;i++)					
			if(pDdz->iPlaCount+1<kPlaMax
				&& pDdz->iOnHand[i]/4==pDdz->iOnHand[i-1]/4					
				&& pDdz->iOnHand[i-1]/4==pDdz->iOnHand[i-2]/4		
				&& pDdz->iOnHand[i-2]/4==pDdz->iOnHand[i-3]/4)
			{ 
				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i-3];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i-2];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i-1];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			}  
}
//H02-END

//H03-START��iOnHand�в�ѯ�������͵�iPlaArr
//����޶���:÷��&�ĺ��н�,����޶�ʱ��:15-03-22
//�޶����ݼ�Ŀ��:�����ܹ�С��
void Help3Single(struct Ddz * pDdz)
{
	int i;
	for(i=0;pDdz->iOnHand[i]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax 
			&& ( pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
				|| pDdz->iOnHand[i] == 53))
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=-1;
			pDdz->iPlaCount++;
		}
	}
}
//H03-END

//H04-START�Ƽ�����Ӧ�Զ���
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-10 12:00 
void Help4Double(struct Ddz * pDdz)
{
	int i;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+1]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4)
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=-1;
			pDdz->iPlaCount++;
		}
	}
}
//H04-END

//H05-START�Ƽ�����Ӧ������
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-10 12:00 
void Help5Three(struct Ddz * pDdz)
{
	int i;
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+2]/4)
		{
			pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
			pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
			pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
			pDdz->iPlaArr[pDdz->iPlaCount][3]=-1;
			pDdz->iPlaCount++;
		}
	}
}
//H05-END

//H0601-START�Ƽ�����Ӧ������һ��
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-10 12:00 
void Help6ThreeOne(struct Ddz * pDdz)
{
	int i,iTemp,j;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+2]/4)
		{
			iTemp = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				//�õ��Ʋ����������е������ҽ�ը���ų�
				if(j == iTemp || j == iTemp+1 || j == iTemp+2 || pDdz->iOnHand[j]/4 == pDdz->iOnHand[i]/4)
					continue;

				pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
				pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
				pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
				pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[j];
				pDdz->iPlaArr[pDdz->iPlaCount][4]=-1;
				pDdz->iPlaCount++;
			}

		}
	}
}
//H0601-END

//H0602-START�Ƽ�����Ӧ������һ��
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-10 12:00 
void Help6ThreeDouble(struct Ddz * pDdz)
{
	int i,iTemp,j;
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+2]>=0;i++)
	{
		if(pDdz->iPlaCount+1<kPlaMax
			&& pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+2]/4)
		{
			iTemp = i;
			for(j = 0 ;pDdz->iOnHand[j+1]>=0 ;j++)
			{
				//�������������е����ų�
				if(j >= iTemp-1 && j <= iTemp+2 )
					continue;
				if(pDdz->iOnHand[j]/4 == pDdz->iOnHand[j+1]/4)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
					pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
					pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
					pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[j];
					pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j+1];
					pDdz->iPlaArr[pDdz->iPlaCount][5]=-1;
					pDdz->iPlaCount++;
				}
			}
		}
	}
}
//H0602-END

//H07-START�Ƽ�����Ӧ�Ե�˳
//����޶���:л��&�ĺ��нܣ�����޶�ʱ��:15-03-20: 11:00
void Help7LinkSingle(struct Ddz * pDdz)
{   int i,j,k,iLength,iTemp,iCount,iBiaoji,iFlag;
	int iCopyCards[21];
	iCount=CountCards(pDdz->iOnHand);
	iLength=pDdz->iLastTypeCount-700;
	SortById(pDdz->iOnHand);
	for(i=0;i<21;i++)
		iCopyCards[i]=pDdz->iOnHand[i];
	for(i=0;iCopyCards[i]>=0;i++)
		if(iCopyCards[i]/4>=12)
			iCopyCards[i]=-1;
	for(i=1;iCopyCards[i]>=0;i++)
		if(iCopyCards[i]/4==iCopyCards[i-1]/4)
			iCopyCards[i-1]=-1;
	for(i=0;i<iCount;i++)
		for(j=i+1;j<iCount;j++)
			if(iCopyCards[i]<iCopyCards[j])
			{ 
				iTemp=iCopyCards[i];
				iCopyCards[i]=iCopyCards[j];
				iCopyCards[j]=iTemp;
			}
	for(i=0;iCopyCards[i]>=0;i++)
		for(j=i+1;iCopyCards[j]>=0;j++)
			if(iCopyCards[i]>iCopyCards[j])
			{ 
				iTemp=iCopyCards[i];
				iCopyCards[i]=iCopyCards[j];
				iCopyCards[j]=iTemp;
			}
			//ȥ���ظ����Ʋ���С��������
	if(pDdz->iLastTypeCount==0)
	{
		for(iLength=5;iLength<=12;iLength++)
		{
				for(i=0;iCopyCards[i+iLength-1]>=0;i++)
				{
				iBiaoji=0;
				iFlag=0;
				if(iCopyCards[i]/4>pDdz->iLastMainPoint)
				{
					for(j=i;j<iLength-1+i;j++)
					{	
						if(iCopyCards[j]/4!=iCopyCards[j+1]/4-1)
						{	
							iBiaoji=1;
							break;
						}
					}
				}
				else
					continue;
				if(pDdz->iPlaCount+1<kPlaMax
					&& iBiaoji==0)
				{
					for(k=i;k<iLength+i;k++)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][iFlag]=iCopyCards[k];
						iFlag++;
						if(k==iLength-1+i)			   
							pDdz->iPlaArr[pDdz->iPlaCount][iFlag++]=-1;
					}
					pDdz->iPlaCount++;
				}
				}
		}
	}
	else
	{
		for(i=0;iCopyCards[i+iLength-1]>=0;i++)
		{
			iBiaoji=0;
			iFlag=0;
			if(iCopyCards[i]/4>pDdz->iLastMainPoint)
			{
				for(j=i;j<iLength-1+i;j++)
				{	
					if(iCopyCards[j]/4!=iCopyCards[j+1]/4-1)
					{	
						iBiaoji=1;
						break;
					}
				}
			}
			else
				continue;
			if(pDdz->iPlaCount+1<kPlaMax
				&& iBiaoji==0)
			{
				for(k=i;k<iLength+i;k++)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][iFlag]=iCopyCards[k];
					iFlag++;
					if(k==iLength-1+i)			   
						pDdz->iPlaArr[pDdz->iPlaCount][iFlag++]=-1;
				}
				pDdz->iPlaCount++;
			}
		}
	}
}
//H07-END

//H08-START�Ƽ�����Ӧ��˫˳
//����޶���:л�ģ�����޶�ʱ��:2015-03-26
void Help8LinkDouble(struct Ddz * pDdz)
{
	int i,j,k,iLength;
	int iCopyCards[21];
	int n=1;
	int iFlag=0;
	int iCardsFlag=0;
	int iCard=0;
	SortById(pDdz->iOnHand);
	iLength=(pDdz->iLastTypeCount-800)/2;
	for(i=0;i<21;i++)
		iCopyCards[i]=pDdz->iOnHand[i];
	for(i=0;iCopyCards[i]>=0;i++)
		if(iCopyCards[i]/4>=12)
			iCopyCards[i]=-1;
	for(i=0;i<CountCards(iCopyCards);i++)
	{
		if(iCopyCards[i]/4!=iCopyCards[i+1]/4)
		{
			for(j=i;j<CountCards(iCopyCards);j++)
				iCopyCards[j]=iCopyCards[j+1];
			i--;
		}

	}//���������� ��������� ���ӱ䵥�� ���ű�û��
	for(i=0;i<CountCards(iCopyCards);i++)
	{
		if(iCopyCards[i]/4==iCopyCards[i+1]/4)
		{
			for(j=i;j<CountCards(iCopyCards);j++)
				iCopyCards[j]=iCopyCards[j+1];
			i--;
		}
	}
	//�����Ͷ��ӱ�ɵ���
	//���Ͻ�����iCopyCards[21]����������ɶ��ӵĵ������������ΪA

	for(i=0;iCopyCards[i]>=0;i++)
	{
		if(iCopyCards[i]/4==iCopyCards[i+1]/4-1)
			n++;
		else
		{
			n=1;
			i=iFlag;
			iFlag++;
		}
		if(n==iLength)
		{
			if(pDdz->iPlaCount+1<kPlaMax
				&& iCopyCards[i+2-iLength]/4>pDdz->iLastMainPoint)
			{
				for(j=i+2-iLength;j<=i+1;j++)
				{
					for (k=0;k<CountCards(pDdz->iOnHand);k++)
					{
						if(iCopyCards[j]/4==pDdz->iOnHand[k]/4&&iCardsFlag<2)
						{
							pDdz->iPlaArr[pDdz->iPlaCount][iCard]=pDdz->iOnHand[k];
							iCard++;
							iCardsFlag++;
						}
					}
					iCardsFlag=0;
				}
				pDdz->iPlaArr[pDdz->iPlaCount][iCard]=-1;
				pDdz->iPlaCount++;
				iCard=0;		
			}
			n=1;
			i=iFlag;
			iFlag++;
		}
	}
}
//H08-END

//H09-START�Ƽ�����Ӧ����˳
//����޶���:�ĺ��нܣ�����޶�ʱ��:2015-03-22
void Help9LinkThree(struct Ddz * pDdz)
{
	int i,iTemp,j,k,iLength = pDdz->iLastTypeCount-900;
	int iTempArr[12][5];
	if(CountCards(pDdz->iOnHand)<6)
		return ;

	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı��룬
	memset(iTempArr,-1,12*5*4);
	SortById(pDdz->iOnHand);    //�Ƚ������������
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48)
		{
			//������Ϊ iCards[i]/4 �����ж����ţ��� j ����ʾ
			j = 1;
			while(-1 != iTempArr[pDdz->iOnHand[i]/4][j])
				j++;

			//���������iTempArr[iCards[i]/4][j]���棬��Ϊ�ñ���ĵ���ΪiCards[i]/4
			iTempArr[pDdz->iOnHand[i]/4][j] = pDdz->iOnHand[i];

			//������Ϊ iCards[i]/4 ���Ƶ����� ��Ϊ j ��
			iTempArr[pDdz->iOnHand[i]/4][0] = j;
		}
	}
	for(i = pDdz->iLastMainPoint+1 ; i <12 ; i++)
	{
		iTemp = iLength;
		if(iTempArr[i][0]>=3)
		{
			j = i;
			while (iTemp)
			{
				if(j < 12 && iTempArr[j][0]>=3)
				{
					iTemp-=3;
					j++;
				}
				else break;
			}
			if(pDdz->iPlaCount+1<kPlaMax
				&& iTemp == 0)
			{
				j--;
				k = 0;
				while (k < iLength)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}
				pDdz->iPlaCount++;
			}
		}
	}
}
//H09-END

//H1001-START�Ƽ�����Ӧ����˳����
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-31
void Help10LinkThreeSingle(struct Ddz * pDdz)
{
	int i,iTemp,j,k,m,iLength = (pDdz->iLastTypeCount-1000);
	int iTempArr[12][5];
	int nFlag = 0;
	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı��룬
	memset(iTempArr,-1,12*5*4);
	SortById(pDdz->iOnHand);    //�Ƚ������������
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48)
		{
			//������Ϊ iCards[i]/4 �����ж����ţ��� j ����ʾ
			j = 1;
			while(-1 != iTempArr[pDdz->iOnHand[i]/4][j])
				j++;

			//���������iTempArr[iCards[i]/4][j]���棬��Ϊ�ñ���ĵ���ΪiCards[i]/4
			iTempArr[pDdz->iOnHand[i]/4][j] = pDdz->iOnHand[i];

			//������Ϊ iCards[i]/4 ���Ƶ����� ��Ϊ j ��
			iTempArr[pDdz->iOnHand[i]/4][0] = j;
		}
	}

	for(i = pDdz->iLastMainPoint+1 ; i <12 ; i++)
	{
		iTemp = iLength;
		if(iTempArr[i][0]>=3)
		{
			j = i;
			while (iTemp)
			{
				if(j < 12 && iTempArr[j][0]>=3)
				{
					iTemp-=4;
					j++;
				}
				else break;
			}
			if(iTemp == 0)
			{
				k = 0;
				j--;
				while (k < iLength/4*3)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}
				
				for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
				{
					nFlag = 1;
					for(m = 0;m < k;m++)
					{
						if(pDdz->iOnHand[j] == pDdz->iPlaArr[pDdz->iPlaCount][m])
						{
							nFlag = 0;
							break;
						}
					}
					if(pDdz->iPlaCount+1<kPlaMax
						&& nFlag)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j];
						k++;
						if(k == iLength)
						{
							pDdz->iPlaCount++;
							break;
						}
					}
				}
			}			
		}
	}
}
//H1001-END

//H1002-START�Ƽ�����Ӧ����˳��˫
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-31
void Help10LinkThreeDouble(struct Ddz * pDdz)
{
	int i,iTemp,j,k,m,iLength = (pDdz->iLastTypeCount-1000);
	int iTempArr[12][5];
	int nFlag = 0;

	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı��룬
	memset(iTempArr,-1,12*5*4);
	SortById(pDdz->iOnHand);    //�Ƚ������������
	for(i = 0 ; pDdz->iOnHand[i] > -1 ; i++)
	{
		if(pDdz->iOnHand[i] < 48)
		{
			//������Ϊ iCards[i]/4 �����ж����ţ��� j ����ʾ
			j = 1;
			while(-1 != iTempArr[pDdz->iOnHand[i]/4][j])
				j++;

			//���������iTempArr[iCards[i]/4][j]���棬��Ϊ�ñ���ĵ���ΪiCards[i]/4
			iTempArr[pDdz->iOnHand[i]/4][j] = pDdz->iOnHand[i];

			//������Ϊ iCards[i]/4 ���Ƶ����� ��Ϊ j ��
			iTempArr[pDdz->iOnHand[i]/4][0] = j;
		}
	}

	for(i = pDdz->iLastMainPoint+1 ; i <12 ; i++)
	{
		iTemp = iLength;
		if(iTempArr[i][0]>=3)
		{
			j = i;
			while (iTemp)
			{
				if(j < 12 && iTempArr[j][0]>=3)
				{
					iTemp-=5;
					j++;
				}
				else break;
			}
			if(iTemp == 0)
			{
				k = 0;
				j--;
				while (k < iLength/5*3)
				{
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][1];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][2];
					k++;
					pDdz->iPlaArr[pDdz->iPlaCount][k]=iTempArr[j][3];
					k++;
					j--;
				}

				for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
				{
					nFlag = 1;
					for(m = 0;m < k;m++)
					{
						if(pDdz->iOnHand[j] == pDdz->iPlaArr[pDdz->iPlaCount][m])
						{
							nFlag = 0;
							break;
						}
					}
					if(pDdz->iPlaCount+1<kPlaMax
						&& nFlag)
					{
						if(pDdz->iOnHand[j]/4 != pDdz->iOnHand[j+1]/4)
							continue;
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j];
						k++;
						pDdz->iPlaArr[pDdz->iPlaCount][k] = pDdz->iOnHand[j+1];
						k++;
						if(k == iLength)
						{
							pDdz->iPlaCount++;
							break;
						}
					}
				}
			}
		}
	}
}
//H1002-END

//H1101-START�Ƽ�����Ӧ���Ĵ�����
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-10 12:00 
void Help11FourSingle(struct Ddz * pDdz)
{
	int i,iTempI,j,k;
	if(CountCards(pDdz->iOnHand)<6)
		return ;
	SortById(pDdz->iOnHand);
	for(i=0;pDdz->iOnHand[i+3]>=0;i++)
	{
		if(pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+2]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+3]/4)
		{
			iTempI = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				//�õ��Ʋ�����4���е���
				if(j == iTempI || j == iTempI+1 || j == iTempI+2 || j == iTempI+3)
					continue;
				for(k = j+1 ; pDdz->iOnHand[k]>=0 ;k++)
				{
					//�õ��Ʋ�����4���е���
					if(k == iTempI || k == iTempI+1 || k == iTempI+2 || k == iTempI+3)
						continue;
					if(pDdz->iPlaCount+1<kPlaMax)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
						pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
						pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
						pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i+3];
						pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j];
						pDdz->iPlaArr[pDdz->iPlaCount][5]=pDdz->iOnHand[k];
						pDdz->iPlaArr[pDdz->iPlaCount][6]=-1;
						pDdz->iPlaCount++;
					}
				}
			}
		}
	}
}
//H1101-END

//H1102-START�Ƽ�����Ӧ���Ĵ�����
//����޶���:�ĺ��нܣ�����޶�ʱ��:15-03-23 12:00 
void Help11FourDouble(struct Ddz * pDdz)
{
	int i,iTempI,j,k;
	if(CountCards(pDdz->iOnHand)<8)
		return ;
	SortById(pDdz->iOnHand);

	for(i=0;pDdz->iOnHand[i+3]>=0;i++)
	{
		if(pDdz->iOnHand[i]/4 > pDdz->iLastMainPoint 
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+1]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+2]/4
			&& pDdz->iOnHand[i]/4 == pDdz->iOnHand[i+3]/4)
		{
			iTempI = i;
			for(j = 0 ;pDdz->iOnHand[j]>=0 ;j++)
			{
				if(j == iTempI || j == iTempI+1 || j == iTempI+2 || j == iTempI+3 ||pDdz->iOnHand[j]/4 != pDdz->iOnHand[j+1]/4)
					continue;
				for(k = j+2 ; pDdz->iOnHand[k]>=0 ;k++)
				{
					//�õ��Ʋ�����4���е����ҰѴ�С���ų���
					if(k == iTempI || k == iTempI+1 || k == iTempI+2 || k == iTempI+3||pDdz->iOnHand[j] >=52 || pDdz->iOnHand[k]/4 != pDdz->iOnHand[k+1]/4)
						continue;
					if(pDdz->iPlaCount+1<kPlaMax)
					{
						pDdz->iPlaArr[pDdz->iPlaCount][0]=pDdz->iOnHand[i];
						pDdz->iPlaArr[pDdz->iPlaCount][1]=pDdz->iOnHand[i+1];
						pDdz->iPlaArr[pDdz->iPlaCount][2]=pDdz->iOnHand[i+2];
						pDdz->iPlaArr[pDdz->iPlaCount][3]=pDdz->iOnHand[i+3];
						pDdz->iPlaArr[pDdz->iPlaCount][4]=pDdz->iOnHand[j];
						pDdz->iPlaArr[pDdz->iPlaCount][5]=pDdz->iOnHand[j+1];
						pDdz->iPlaArr[pDdz->iPlaCount][6]=pDdz->iOnHand[k];
						pDdz->iPlaArr[pDdz->iPlaCount][7]=pDdz->iOnHand[k+1];
						pDdz->iPlaArr[pDdz->iPlaCount][8]=-1;
						pDdz->iPlaCount++;
					}
				}
			}
		}
	}
}
//H1102-END

//MO1-START���Ʊ�����������
//����޶���:�ĺ��н�&÷��,����޶�ʱ��:15-02-09
void SortById(int iCards[])
{
	int i, j;
	int iTemp;
	for (i = 0; iCards[i] >= 0; i++)
	{
		for (j = i + 1; iCards[j] >= 0; j++)
			if (iCards[i]>iCards[j])
			{
				iTemp = iCards[i];
				iCards[i] = iCards[j];
				iCards[j] = iTemp;
			}
	}
}
//MO1-END

//M02-START������(ͬ������)������������
//����޶���:�ĺ��н�,����޶�ʱ��:2015-03-01
void SortByMuch(int iCards[])
{
	int i,j,k,n;
	// iTempArr[i][0]����װ����Ϊ i �ж������ƣ�
	// iTempArr[i][1],iTempArr[i][2],iTempArr[i][3],iTempArr[i][4]����װ����Ϊ i ���Ƶı��룬
	int iTempArr[13][5];
	memset(iTempArr,-1,13*5*4);
	SortById(iCards);    //�Ƚ�iCards�����������
	for(i = 0 ; iCards[i] > -1 ; i++)
	{
		if(iCards[i] < 52)
		{
			//������Ϊ iCards[i]/4 �����ж����ţ��� j ����ʾ
			j = 1;
			while(-1 != iTempArr[iCards[i]/4][j])
				j++;
			//���������iTempArr[iCards[i]/4][j]���棬��Ϊ�ñ���ĵ���ΪiCards[i]/4
			iTempArr[iCards[i]/4][j] = iCards[i];
			//������Ϊ iCards[i]/4 ���Ƶ����� ��Ϊ j ��
			iTempArr[iCards[i]/4][0] = j;
		}
	}
	n = 0;   //nΪiCards���±꣬���½�iTempArr�е�������iCards��
	for(i = 4 ; i > 0 ;i--) //���ҳ�iTempArrһ���������ƣ�������д��ԭ����iCards�����У�Ȼ����Ѱ�����ŵģ�һ������	
	{
		for(j = 0 ;j < 13 ; j++)
		{
			if(iTempArr[j][0] == i)    //�жϸõ��������ǲ����� i ��
			{
				for(k = 1;k <= i ; k++)    // �еĻ����Ͱ��ƶ��Ž�iCards[ n ]�У�Ȼ�� n++
				{
					iCards[n] = iTempArr[j][k];
					n++;
				}
			}
		}
	}
}
//MO2-END

//M03-START��������Ҫ������������
//����޶���:�ĺ��н�,����޶�ʱ��:15-03-08 18:00 
void SortByMain(int iCards[])
{
	if(IsType0Pass(iCards))
		return ;
	if(IsType1Rocket(iCards))
		return ;
	if(IsType2Bomb(iCards))
		return ;
	if(IsType3Single(iCards))
		return ;
	if(IsType4Double(iCards))
		return ;
	if(IsType5Three(iCards))
		return ;
	if(IsType6ThreeOne(iCards))
		return ;
	if(IsType6ThreeDouble(iCards)) 
		return ;
	if(IsType7LinkSingle(iCards))
		return ;
	if(IsType8LinkDouble(iCards))
		return ;
	if(IsType9LinkThree(iCards))
		return ;
	if(IsType10LinkThreeSingle(iCards))
		return ;
	if(IsType10LinkThreeDouble(iCards))
		return ;
	if(IsType11FourSingle(iCards))
		return ;
	if(IsType11FourDouble(iCards))
		return ;
	return;
}
//MO3-END

//M04-START���ñ������ݳ�ʼֵ
//����޶���:÷��&�ĺ��н�,����޶�ʱ��:15-04-05
//�޸�Ŀ��:iLastPasCount,iLastTypeCount,iLastMainPoint��ʼ��
void InitRound(struct Ddz * pDdz)	
{
	int i, j;
	pDdz->iStatus = 1;					//��ʼ����������״̬
	strcpy(pDdz->sCommandIn,"");		//��ʼ������ͨ����������
	strcpy(pDdz->sCommandOut,"");		//��ʼ������ͨ���������
	for (i = 0; i < 21; i++)			//��ʼ����������
		pDdz->iOnHand[i] = -1;
	for (i = 0; i<162; i++)				//��ʼ������������
		for (j = 0; j<21; j++)
			pDdz->iOnTable[i][j] = -2;
	for (i = 0; i < 21; i++)			//��ʼ�����ֳ���
		pDdz->iToTable[i] = -1;
	strcpy(pDdz->sVer,"");				//��ʼ������Э��汾��
	strcpy(pDdz->sVer,kPlayerName);		//��ʼ�����ֲ���ѡ�ֳƺ�
	pDdz->cDir='B';						//��ʼ��������ҷ�λ���
	pDdz->cLandlord='B';				//��ʼ�����ֵ�����λ���
	pDdz->cWinner='B';					//��ʼ������ʤ�߷�λ���
	for (i = 0; i < 3; i++)				//��ʼ�����ֽ���
		pDdz->iBid[i] = -1;
	pDdz->iLastPassCount=2;		//��ǰ��������PASS����ֵ��[0,2],��ֵ2����������ȡ0��һ��PASSȡ1������PASSȡ2��
	pDdz->iLastTypeCount=0;		//��ǰ��������������ֵ��[0,1108],��ֵ0��iLastPassCount=0ʱ����ֵ��=1ʱ����ԭֵ��=2ʱֵΪ0��
	pDdz->iLastMainPoint=-1;		//��ǰ�������Ƶ�����ֵ��[0,15],��ֵ-1��iLastPassCount=0ʱ����ֵ����=1ʱ����ԭֵ��=2ʱֵΪ-1��
	pDdz->iBmax = 0;					//��ʼ�����ֽ��Ʒ���
	pDdz->iOTmax = 0;					//��ʼ����������������
}
//MO4-END

//M05-START����������ת��Ϊ�ַ���׷��(-1��)
//����޶���:÷��,����޶�ʱ��:15-03-01
void AppendCardsToS(int iCards[],char sMsg[])
{
	int i;
	char sCard[4]="";
	char sCardString[90]="";
	if (iCards[0] == -1)	// PASS
		strcat(sCardString, "-1");
	else					// ����PASS
	{
		for (i = 0; iCards[i] >= 0; i++)
		{
			if (iCards[i] >= 10)
			{
				sCard[0] = iCards[i] / 10 + '0';
				sCard[1] = iCards[i] % 10 + '0';
				sCard[2] = ',';
				sCard[3] = '\0';
			}
			else
			{
				sCard[0] = iCards[i] % 10 + '0';
				sCard[1] = ',';
				sCard[2] = '\0';
			}
			strcat(sCardString, sCard);
		}
		sCardString[strlen(sCardString) - 1] = '\0';	//ȥ�������β������
		
	}
	strcat(sMsg, sCardString);
}
//MO5-END
/*�˺����ѱ�Hansen���������ƺ���ȡ�� 
//M06-START�����Ƽ�����
//����޶���:÷��&�ĺ��нܣ�����޶�ʱ��:15-03-23
void HelpPla(struct Ddz * pDdz)
{
	int i,j;
	for(i=0;i<kPlaMax;i++)		//����
		for(j=0;j<21;j++)
			pDdz->iPlaArr[i][j]=-1;
	pDdz->iPlaCount=0;
	if(pDdz->iLastTypeCount==0)	//����2��PASS,����������
	{
		Help0Pass(pDdz);
		return;
	}
	else
	{
		Help1Rocket(pDdz);
		Help2Bomb(pDdz);
		if(301 == pDdz->iLastTypeCount)//����
			Help3Single(pDdz);
		else if(402 == pDdz->iLastTypeCount)
			Help4Double( pDdz);
		else if(503 == pDdz->iLastTypeCount)
			Help5Three(pDdz);
		else if(604 == pDdz->iLastTypeCount)
			Help6ThreeOne(pDdz);
		else if(605 == pDdz->iLastTypeCount)
			Help6ThreeDouble(pDdz);
		else if(pDdz->iLastTypeCount > 700 && pDdz->iLastTypeCount < 800)
			Help7LinkSingle(pDdz);
		else if(pDdz->iLastTypeCount > 800 && pDdz->iLastTypeCount < 900)
			Help8LinkDouble(pDdz);
		else if(pDdz->iLastTypeCount > 900 && pDdz->iLastTypeCount < 1000)
			Help9LinkThree(pDdz);
		else if(pDdz->iLastTypeCount > 1000 && pDdz->iLastTypeCount < 1100)
		{
			if((pDdz->iLastTypeCount-1000)%4 == 0)
				Help10LinkThreeSingle(pDdz);
			else
				Help10LinkThreeDouble(pDdz);
		}
		else if(1106 == pDdz->iLastTypeCount)
			Help11FourSingle(pDdz);
		else if(1108 == pDdz->iLastTypeCount)
			Help11FourDouble(pDdz);
		return;
	}
}
//MO6-END
*/
//M07-START������ģ���Ƴ�һ����
//����޶���:÷��,����޶�ʱ��:15-03-01
void HelpTakeOff(struct Ddz *  pDdz,int num)
{
	int i,j = 0,k,same;
	for(i = 0; pDdz->iOnHand[i]>=0 ;i++)
	{
		same = 0;
		for(k = 0 ;pDdz->iPlaArr[num][k] >=0 ; k++)
		{
			if(pDdz->iOnHand[i] == pDdz->iPlaArr[num][k])
			{
				same = 1;
			}
		}
		if(same!=1)
		{
			pDdz->iPlaOnHand[j] = pDdz->iOnHand[i];
			j++;
		}
	}
	pDdz->iPlaOnHand[j] = -1;
}
//MO7-END


//P01-START���ñ������ݳ�ʼֵ
//����޶���:÷��,����޶�ʱ��:15-02-08 00:13 
void InitTurn(struct Ddz * pDdz)	
{
	pDdz->iTurnTotal=1;				//��ʼ��������
	pDdz->iTurnNow=1;					//��ʼ����ǰ�ִ�
	pDdz->iRoundTotal=1;				//��ʼ���ܾ���
	pDdz->iRoundNow=1;				//��ʼ����ǰ�ִ�
	InitRound(pDdz);					//��ʼ����������
}
//P01-END

//P02-START������Ϣ
//����޶���:÷��,����޶�ʱ��:15-02-08
void InputMsg(struct Ddz * pDdz, string pCmd, bool selfPlay = true)	
{
	if(selfPlay){
        //	cin.getline(pDdz->sCommandIn,80);
		string s = pCmd;
		//	cout<<s<<endl;
		strcpy(pDdz->sCommandIn, s.c_str());
		//	if(pDdz->ssCommandIn[0] == 'D' && pDdz->ssCommandIn[1] == 'e')
		//		cin.getline(pDdz->sCommandIn,80);
	}
	else{
		cin.getline(pDdz->sCommandIn,80);
	}
}
//P02-END

//P03-START����������Ϣ
//����޶���:÷��,����޶�ʱ��:15-02-08 00:13 
void AnalyzeMsg(struct Ddz * pDdz)	
{
	char sShort[4];
	int i;
	for(i=0;i<3;i++)
		sShort[i]=pDdz->sCommandIn[i];
	sShort[3]='\0';
	if(strcmp(sShort,"DOU")==0)					//�汾��Ϣ
	{
		GetDou(pDdz);
		return;
	}
	if(strcmp(sShort,"INF")==0)					//�־���Ϣ
	{
		GetInf(pDdz);
		return;
	}
	if(strcmp(sShort,"DEA")==0)					//������Ϣ
	{
		GetDea(pDdz);
		return;
	}
	if(strcmp(sShort,"BID")==0)					//���ƹ���
	{
		GetBid(pDdz);
		return;
	}
	if(strcmp(sShort,"LEF")==0)					//������Ϣ
	{
		GetLef(pDdz);
		return;
	}
	if(strcmp(sShort,"PLA")==0)					//���ƹ���
	{
		GetPla(pDdz);
		return;
	}
	if(strcmp(sShort,"GAM")==0)					//ʤ����Ϣ
	{
		GetGam(pDdz);
		return;
	}
	if(strcmp(sShort,"EXI")==0)					//ǿ���˳�
	{
		exit(0);
	}
	strcpy(pDdz->sCommandOut,"ERROR at module AnalyzeMsg,sCommandIn without match");
	return ;
}
//P03-END

//P0301-START��ȡ�������汾��ϢDOU
//����޶���:�ź���&÷��,����޶�ʱ��:15-02-10 21:04 
//�޶����ݼ�Ŀ��:�޶�kPlayerNmae
void GetDou(struct Ddz * pDdz)
{
	int i;								//��ѭ������
	for (i = 0; pDdz->sCommandIn[i] != '\0'; i++)
		pDdz->sVer[i] = pDdz->sCommandIn[i];
	pDdz->sVer[i] = '\0';
	strcpy(pDdz->sCommandOut,"NAME ");
	strcat(pDdz->sCommandOut,kPlayerName);
}
//P0301-END

//P0302-START��ȡ�������־���ϢINF
//����޶���:�ų�&÷��,����޶�ʱ��:15-02-10 
void GetInf(struct Ddz * pDdz)		//�ִ���Ϣ��������(����������Ϣ����Ӧд������Ա������):����INFO 1/4 1/9 9 2450     ���OK INFO
{
	char c;					//�浱ǰ�ֽ���Ϣ		
	int iCount=0;			//��¼���ݸ���
	int iTemp=0;			//�м����
	int iMessage[6]={0};		//��¼��������
	int i;
	for(i=5;pDdz->sCommandIn[i] != '\0';i++)
	{
		c= pDdz->sCommandIn[i];	
		if(c>='0' && c<='9')											//��ǰ�ַ�Ϊ0-9
		{
			iTemp = iTemp * 10 + c - '0';
			iMessage[iCount] = iTemp;								//����ѡ����
		}
		if(c==',')														//��ǰ�ַ�Ϊ����,
		{
			iCount++;
			iTemp=0;
		}
	}
	pDdz->iTurnNow = iMessage[0];						//��ǰ�ִ�
	pDdz->iTurnTotal = iMessage[1];						//������
	pDdz->iRoundNow = iMessage[2];						//��ǰ�ִ�
	pDdz->iRoundTotal =iMessage[3];						//�ܾ���
	pDdz->iLevelUp=iMessage[4];							//��������
	pDdz->iScoreMax =iMessage[5];						//�÷ֽ���
	strcpy(pDdz->sCommandOut, "OK INFO");
}
//P0302-END

//P0303-START��ȡ������������ϢDEA
//����޶���:����&÷��,����޶�ʱ��:15-02-09 22:55 
//�޶����ݼ�Ŀ��:�޸Ķ����㷨
void GetDea(struct Ddz * pDdz)	
{
	int i;			      //��ѭ������
	int iNow = 0;		  //��ǰ���������
	int iCardId = 0;	//��ǰ�����Ʊ���
	char c;			      //��ǰָ���ַ�
	pDdz->cDir = pDdz->sCommandIn[5];     //��ȡ����AI��λ���
	for(i=0 ; i<21 ; i++)				          //����iOnhand[]
		pDdz->iOnHand[i] = -1;
	for(i=6 ; pDdz->sCommandIn[i] != '\0'; i++)	//���ζ�ȡ����ָ��
	{
		c = pDdz->sCommandIn[i];			      //cΪ��ǰָ���ַ�
		if(c>='0' && c<='9')				        //��ǰ�ַ�Ϊ0-9
		{
			iCardId = iCardId * 10 + c - '0';
			pDdz->iOnHand[iNow] = iCardId;
		}
		if(c==',')							            //��ǰ�ַ�Ϊ����,
		{
			iCardId = 0;
			iNow++;
		}
	}
	strcpy(pDdz->sCommandOut, "OK DEAL");  //�ظ���Ϣ
	SortById(pDdz->iOnHand);  //iOnHand[]��С��������
}
//P0303-END

//P0304-START��ȡ������������ϢBID
//����޶���:��˼��&÷��,����޶�ʱ��:15-02-08
void GetBid(struct Ddz * pDdz)	
{
	if(pDdz->sCommandIn[4]=='W')					//���������ϢΪBID WHAT
	{
		strcpy(pDdz->sCommandOut,"BID _0");
		pDdz->sCommandOut[4]=pDdz->cDir;
		pDdz->iBid[pDdz->cDir-'A']=CalBid(pDdz);
		pDdz->sCommandOut[5]=pDdz->iBid[pDdz->cDir-'A']+'0';		//���ý��ƺ���
		pDdz->sCommandOut[6]='\0';
		return ;
	}
	if(pDdz->sCommandIn[4]>='A'&&pDdz->sCommandIn[4]<='C')  //������ϢΪBID **
	{
		pDdz->iBid[pDdz->sCommandIn[4]-'A']=pDdz->sCommandIn[5]-'0';
		strcpy(pDdz->sCommandOut,"OK BID");
		return;
	}
}
//P0304-END

//P0305-START��ȡ������������ϢLEF
//����޶���:����&÷��,����޶�ʱ��:15-02-08
void GetLef(struct Ddz * pDdz)	
{
	int i, iCount = 0;
	char c;
	pDdz->cLandlord = pDdz->sCommandIn[9];    //ȷ��������
	pDdz->iLef[0]=0;
	pDdz->iLef[1]=0;
	pDdz->iLef[2]=0;
	for (i = 10; pDdz->sCommandIn[i] != '\0'; i++)
	{
		c=pDdz->sCommandIn[i];
		if (c>='0' && c<='9')
			pDdz->iLef[iCount] = pDdz->iLef[iCount] * 10 + c - '0';
		else
			iCount++;
	}
	if (pDdz->sCommandIn[9] == pDdz->cDir)
	{
		pDdz->iOnHand[17] = pDdz->iLef[0];
		pDdz->iOnHand[18] = pDdz->iLef[1];
		pDdz->iOnHand[19] = pDdz->iLef[2];
		pDdz->iOnHand[20] = -1;
	}
	strcpy(pDdz->sCommandOut, "OK LEFTOVER");
	SortById(pDdz->iOnHand);					//iOnHand[]��С��������
}
//P0305-END

//P0306-START��ȡ������������ϢPLA
//����޶���:÷��,����޶�ʱ��:15-02-08 
void GetPla(struct Ddz * pDdz)	
{
	if(pDdz->sCommandIn[5]=='W')					//������ϢΪPLAY WHAT��Ӧ���ó��Ƽ��㺯���������
	{
		CalPla(pDdz);					//���ó��Ƽ��㺯���������
		strcpy(pDdz->sCommandOut,"PLAY _");
		pDdz->sCommandOut[5]=pDdz->cDir;		//��������Ԥ����Ϣ׼����sCommandOut����
		AppendCardsToS(pDdz->iToTable,pDdz->sCommandOut);		//Ҫ��������iToTable[]�е�����ת��Ϊ�ַ������ӵ�sCommandOut��
		UpdateMyPla(pDdz);		//���ݼ������Ƹ�������
	}
	else										//�����յ���ϢΪ������ҳ���
	{
		UpdateHisPla(pDdz);		//�������˳��Ƹ�������       
		strcpy(pDdz->sCommandOut,"OK PLAY");//�ظ��յ�
	}
	//��ǰ������1
	pDdz->iOTmax++;
}
//P0306-END

//P030602-START���ݼ������Ƹ�������
//����޶���:÷��&�ĺ��н�,����޶�ʱ��:15-03-01
//�޶����ݼ�Ŀ��:�޸ļ���������
void UpdateMyPla(struct Ddz * pDdz)	
{
	int i,j,k;
	//cout << pDdz->cDir <<endl;
	if(pDdz->iToTable[0]==-1)	//Pass
	{
		pDdz->iOnTable[pDdz->iOTmax][0]=-1;
		pDdz->iLastPassCount++;
		if(pDdz->iLastPassCount>=2)	//��������PASS
		{
			pDdz->iLastPassCount=0;
			pDdz->iLastTypeCount=0;
			pDdz->iLastMainPoint=-1;
		}
		//pDdz->iOTmax++;
	}
	else						//����PASS
	{
		//����������
		for(i=0;pDdz->iToTable[i]>=0;i++)
			pDdz->iOnTable[pDdz->iOTmax][i]=pDdz->iToTable[i];
		pDdz->iOnTable[pDdz->iOTmax][i]=-1;
		//pDdz->iOTmax++;
		//����������
	
		for(j=0 ; pDdz->iToTable[j]>=0 ; j++)
		{	
			for(i=0;pDdz->iOnHand[i]>=0;i++)
				if(pDdz->iOnHand[i]==pDdz->iToTable[j])
				{
					for(k=i;pDdz->iOnHand[k]>=0;k++)
						pDdz->iOnHand[k]=pDdz->iOnHand[k+1];
					break;
				}
		}
		pDdz->iLastPassCount=0;
		pDdz->iLastTypeCount=AnalyzeTypeCount(pDdz->iToTable);
		pDdz->iLastMainPoint=AnalyzeMainPoint(pDdz->iToTable);
	}
}
//PO30602-END

//P030603-START�������˳��Ƹ�������
//����޶���:÷��,����޶�ʱ��:15-03-01
void UpdateHisPla(struct Ddz * pDdz)	
{
	int i;
	int iCardId;
	int iNow;
	char c;
	//����������
	if(pDdz->sCommandIn[6]=='-')// PLAY ?-1 ��PASS
	{
		pDdz->iOnTable[pDdz->iOTmax][0]=-1;
		pDdz->iLastPassCount++;
		if(pDdz->iLastPassCount>=2)	//��������PASS
		{
			pDdz->iLastPassCount=0;
			pDdz->iLastTypeCount=0;
			pDdz->iLastMainPoint=-1;
		}
		//pDdz->iOTmax++;
	}
	else						// PLAY ����
	{
		for(i=0 ; i<21 ; i++)							//����iToTable[]
			pDdz->iToTable[i] = -1;
		iCardId=0;
		iNow=0;
		for(i=6 ; pDdz->sCommandIn[i] != '\0'; i++)		//���ζ�ȡ����
		{
			c = pDdz->sCommandIn[i];					//cΪ��ǰָ���ַ�
			if(c>='0' && c<='9')				        //��ǰ�ַ�Ϊ0-9
			{
				iCardId = iCardId * 10 + c - '0';
				pDdz->iToTable[iNow] = iCardId;
			}
			if(c==',')									//��ǰ�ַ�Ϊ����,
			{
				iCardId = 0;
				iNow++;
			}
		}
		//����������
		for(i=0;pDdz->iToTable[i]>=0;i++)
			pDdz->iOnTable[pDdz->iOTmax][i]=pDdz->iToTable[i];
		pDdz->iOnTable[pDdz->iOTmax][i]=-1;
		pDdz->iLastPassCount=0;
		pDdz->iLastTypeCount=AnalyzeTypeCount(pDdz->iToTable);
		pDdz->iLastMainPoint=AnalyzeMainPoint(pDdz->iToTable);
		//pDdz->iOTmax++;
	}
}
//PO30603-END


//P0307-START��ȡ������ʤ����ϢGAM
//����޶���:÷��,����޶�ʱ��:15-02-08 00:13 
void GetGam(struct Ddz * pDdz)	
{
	pDdz->cWinner = pDdz->sCommandIn[9];			//ʤ���߷�λ���
	if(pDdz->iRoundNow == pDdz->iRoundTotal)		//�����ǰ������ÿ�־����ʱ
	{
		pDdz->iStatus = 0;							//����״̬������Ϊ����
	}
	else											//����
	{
		pDdz->iRoundNow++;							//��ǰ�ִμ�1
		InitRound(pDdz);							//����״̬������Ϊ���¿�ʼ��һ��
	}
	strcpy(pDdz->sCommandOut, "OK GAMEOVER");
}
//PO307-END

//P04-START�����Ϣ
//����޶���:÷��,����޶�ʱ��:15-02-08 00:13 
void OutputMsg(struct Ddz * pDdz, vector<string>& answer, bool selfPlay = true)	
{
	if(selfPlay){
		//	cout<<pDdz->sCommandOut<<endl;
		string s(pDdz->sCommandOut);
		answer.push_back(s);
		//dout<<pDdz->sCommandOut<<endl;
	}
	else{
		cout<<pDdz->sCommandOut<<endl;
	    //dout<<pDdz->sCommandOut<<endl;
	}
}
//P04-END

//P05-START������������
//����޶���:÷��,����޶�ʱ��:15-02-08
void CalOthers(struct Ddz * pDdz)	
{
	pDdz->iVoid=0;
}
//P05-END
