#define kPlayerName "TeamA"
#define kPlaMax 500
#include "DdzV200.h"
#include <assert.h>
#include "opponent_model.h"
#include "ismcts.h"

//
#define DLLEXPORT extern "C" __declspec(dllexport)

vector<int> othersCards(15);//������
vector<int> remainCards(54);//������2��

int AtoB_dedication=0;
int cardRemaining[3];//��������ͳ��
int DirConvertPara;//λ�ý�������
int myPosition; //���ַ�λ 0,1,2 Landlord,FarmerA,FarmerB
int lastPlayer; //��һλ�Ϸ���ҵ����ַ�λ
int lastTurn;	//��һ����Ч���͵�����
bool iCanWin;
int currentPlayer; //ָʾÿ�ִ���һ����ʼ
vector<float> TrainParams;		//��ѵ���Ĺ�ֵ��������


//�Ż��ò���
float baseCardsWeight = 1.0;            //�����Ƶ�Ȩ��
float roundValWithDecay = 7;          //����������Ȩ��
float SeqDecayRate =  1;			    //����˳�ӵ�Ȩ��
float RocketVal = 20;
vector<float> param_vec;				//��������

int sum = 0;			//���Ļ�õ��ܷ���

int invalidRound = 0;			//��Ч�ľ���

//���������
int channels = 21, height = 19, width = 15;
int batch_size = 32;
int num_classes = 309;
int dim1 = height * width; int &dim2 = width;

//��ʼ��
//vector<float> feature(channels * height * width, 0);
int * feature;
//vector<float> zero_vec(channels * height * width, 0);
vector<int> limit = {-1, 5, 3, 2};
vector<int> len_vec(4, 0);
vector<int> zero_len(4, 0);
vector<int> zeros(15, 0);


//ģ���ñ���
vector<string> deliveredCards(4, "default");	//������ƽ��
vector<string> answer;		//��һ�Ӧ��Ϣ����

// �Ƶ���ϣ����ڼ�������
typedef struct CardCombo
{
	// ��ʾͬ�ȼ������ж�����
	// �ᰴ�����Ӵ�С���ȼ��Ӵ�С����
	struct CardPack
	{
		Level level;
		short count;
		bool operator< (const CardPack& b) const
		{
			if (count == b.count)
				return level > b.level;
			return count > b.count;
		}
	};
	vector<Card> cards; // ԭʼ���ƣ�δ����
	vector<CardPack> packs; // ����Ŀ�ʹ�С���������
	CardComboType comboType; // ���������
	Level comboLevel = 0; // ����Ĵ�С��
	bool operator< (const CardCombo& b) const
	{
		if(comboType == CardComboType::BOMB or comboType == CardComboType::ROCKET) return false;
		if(comboType == b.comboType){
			return comboLevel < b.comboLevel;
		}
		else{
			return comboType > b.comboType;
		}
	}
	float getValue() const;//��ȡvalue
	float getValue2() const;//��ȡvalue
	float getSeqValue(comboDiv& ComboSeq);//��ȡ������value
	/*---------------�����ظ������ĺ���---------------*/
	float MergeValue(CardCombo main, comboDiv DivRemain);
	CardCombo MergeCombo(comboDiv& singles, comboDiv& pairs);	//�ϲ�����
	void ValueCompare(vector<Card>& divcards,vector<Card>& cuts, comboDiv& DivRemain, float& MaxValue);
	bool RemainValueCompare(vector<Card>& remainCards, ComboDiv& FinalSeq, CardCombo& myAction);
	float getExValue(comboDiv& Seq, short mainlv, short dlv, short k);
	/*���������CardPack�ݼ��˼���,���ڼ���˳��ʲô�� */
	int findMaxSeq() const;
	Seq findSeq1() const; //��ⵥ˳
	Seq findSeq2() const; //���˫˳
	Seq findSeq3() const; //���ɻ�
	Seq findSeq4() const; //��⺽��ɻ�
	CardCombo() : comboType(CardComboType::PASS) {} // ����һ��������
	//����������
	ComboDiv div(const CardCombo& remains){
		if(remains.comboType == CardComboType::PASS){//��������ˣ��Ǿ�ֱ�ӷ��ؿ�������
			ComboDiv FinalDiv;
			FinalDiv.Value = 0;
			return FinalDiv;
		}
		else{
			// ÿ�����ж��ٸ�
			short counts[MAX_LEVEL + 1] = {};

			// ͬ���Ƶ��������ж��ٸ����š����ӡ�������������
			short countOfCount[5] = {};

			Level check[MAX_LEVEL+1];
			for (Card c : remains.cards)//��Ͱ�Ż�ԭ����
				counts[card2level(c)]++;
			for (Level l = 0; l <= MAX_LEVEL; l++)
				if (counts[l])
					countOfCount[counts[l]]++;
			// ���������������ǿ��ԱȽϴ�С��
			comboLevel = remains.packs[0].level;

			// ��������
			// ���� ͬ���Ƶ����� �м��� ���з���
			vector<int> kindOfCountOfCount;
			for (int i = 0; i <= 4; i++)
				if (countOfCount[i])
					kindOfCountOfCount.push_back(i);
			sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

			int curr;
			vector<Card> divcards = remains.cards;//����׷�ݵ���
			vector<Card> cuts;		//��ȡ����
			CardCombo CutCombo;		//��ȡ������ɵ�Combo
			ComboDiv FinalDiv;		//���յķ���ֵ
			comboDiv DivRemain;  	//����Combo����
			comboDiv TempSeq;		//�ݴ�������͵���ʱ����
			float MaxValue = -1000;		//�ݴ�Value�����Value
			Seq seq1 = remains.findSeq1();
			Seq seq2 = remains.findSeq2();
			Seq seq3 = remains.findSeq3();
			Seq seq4 = remains.findSeq4();
			/*
				Ŀ�ģ���ȡһ�����ƣ�ʣ�µļ����֣�ֱ������������
				����ֵ���Ѿ����������
			*/
			switch (kindOfCountOfCount.size())
			{
			case 1: // ֻ��һ����
				curr = countOfCount[kindOfCountOfCount[0]];
				switch (kindOfCountOfCount[0])
				{
				case 1:
					// ֻ�����ɵ���,��������ը��˳��
					if(curr >= 2 && remains.packs[1].level == level_joker){
						//��ʼ������~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));

						for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�Ҵ�С����cutһ��
						{
							if(card2level(*c) == level_joker or card2level(*c) == level_JOKER){//�ҵ���С����ת��
								cuts.push_back(*c);
								c = divcards.erase(c); //���б�ɾ
							}
							else c++;
						}
						//���͵ݹ鲿
						ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					if(seq1.len >= 5  && seq1.level<= MAX_STRAIGHT_LEVEL){//��˳
						//��ʼ������~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
							curr = seq1.len;
							for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�ҵ�˳��cutһ��
							{
								Level tmplv = card2level(*c);
								if(tmplv > seq1.level - curr && tmplv<=seq1.level && check[tmplv]<1 ){//�ҵ�˳�����о�ת�ơ�����
									cuts.push_back(*c);
									c = divcards.erase(c); //���б�ɾ
									check[tmplv]++;
									if(cuts.size()>=5){
										ValueCompare( divcards, cuts, DivRemain, MaxValue);
									}
								}
								else c++;
							}
					//	ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					{//��ʣ������
						//��ʼ������~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));

						cuts.push_back(*(divcards.begin())); //����cut��һ��
						divcards.erase(divcards.begin());
						//���͵ݹ鲿
						ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					break;
				case 2:
					// ֻ�����ɶ���
					if(seq2.len >= 3  && seq2.level<= MAX_STRAIGHT_LEVEL){//˫˳
						//��ʼ������~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
							curr = seq2.len;
							for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��˫˳��cutһ��
							{
								Level tmplv = card2level(*c);
								if(tmplv > seq2.level - curr && tmplv<=seq2.level && check[tmplv]<2 ){//�ҵ�˳�����о�ת�ơ�˫��
									cuts.push_back(*c);
									c = divcards.erase(c); //���б�ɾ
									check[tmplv]++;
									if(cuts.size()>=6 && cuts.size() % 2 == 0){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
								}
								else c++;
							}

					}
					{//���ǶԶ�
						//��ʼ������~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
						for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�Ҷ��ӣ�cutһ��
						{
							if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
								cuts.push_back(*c);
								c = divcards.erase(c); //���б�ɾ
								if(cuts.size()>=2){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
							}
							else c++;
						}

					}
					break;
				case 3:// ֻ����������
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��������cutһ��
					{

						if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							if(cuts.size()>=3){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
						}
						else c++;

					}
					break;
				case 4:// ֻ������ը��
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��ը����cutһ��
					{
						if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
						}
						else c++;
					}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);
					break;
				}
				break;
			case 4: // �ж�����
			case 3:
			case 2:
				curr = countOfCount[kindOfCountOfCount[1]];
				if(seq4.len >= 2  && seq4.level<= MAX_STRAIGHT_LEVEL){//����ɻ�������
					//��ʼ������~~
					/*
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
						curr = seq4.len;
						for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�Һ���ɻ����ƣ�cutһ��
						{
							Level tmplv = card2level(*c);
							if(tmplv > seq4.level - curr && tmplv<=seq4.level && check[tmplv]<4 ){//�ҵ�˳�����о�ת�ơ��ġ�
								cuts.push_back(*c);
								c = divcards.erase(c); //���б�ɾ
								check[tmplv]++;
							}
							else c++;
						}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);*/
				}
				if(countOfCount[4]){
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��ը����cutһ��
					{
						if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
						}
						else c++;
					}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);
				}
				if(seq3.len >= 2  && seq3.level<= MAX_STRAIGHT_LEVEL){//�ɻ�������
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq3.len;
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�ҷɻ����ƣ�cutһ��
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq3.level - curr && tmplv<=seq3.level && check[tmplv]<3 ){//�ҵ�˳�����о�ת�ơ�����
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							check[tmplv]++;
							if(cuts.size()>=6 && cuts.size() % 3 == 0){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}
				}

				if(countOfCount[3])// ֻ����������
				{
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��������cutһ��
					{
						if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							if(cuts.size()>=3){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}

				}

				if(seq2.len >= 3  && seq2.level<= MAX_STRAIGHT_LEVEL){//˫˳
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq2.len;
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ��˫˳��cutһ��
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq2.level - curr && tmplv<=seq2.level && check[tmplv]<2 ){//�ҵ�˳�����о�ת�ơ�˫��
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							check[tmplv]++;
							if(cuts.size()>=6 && cuts.size() % 2 == 0){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;

					}
				}

				if(seq1.len >= 5  && seq1.level<= MAX_STRAIGHT_LEVEL){//��˳
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq1.len;
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�ҵ�˳��cutһ��
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq1.level - curr && tmplv<=seq1.level && check[tmplv]<1 ){//�ҵ�˳�����о�ת�ơ�����
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							check[tmplv]++;
							if(cuts.size()>=5){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}
				}
				if(countOfCount[2])// ֻ�����ɶԶ�
				{
					//��ʼ������~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //Ѱ�Ҷ�����cutһ��
					{
						if(card2level(*c) == remains.packs[0].level){//�Ǿ��ҵ�һ���
							cuts.push_back(*c);
							c = divcards.erase(c); //���б�ɾ
							if(cuts.size()>=2){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}

				}
				break;
			}
			FinalDiv.div = DivRemain;
			FinalDiv.Value = MaxValue;
			return FinalDiv;
		}
	}
	/*ͨ��Card����short�����͵ĵ���������һ������,��������ͺʹ�С���
	 * ��������û���ظ����֣����ظ���Card��*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end);
	/*�ж�ָ�������ܷ�����ǰ���飨������������ǹ��Ƶ��������*/
	bool canBeBeatenBy(const CardCombo& b) const;
}CARDCOMBO;
CardCombo HelpPass(struct Ddz * pDdz);	//��������
void setOthersCards( Ddz * pDdz );		//����ͳ��
//�����Ƴ�������vector��HelpTakeOff
void removeCards(vector<Card>& ownCards, vector<Card>& deleteCards, vector<Card>& remainCards);
void infoConvert( Ddz * pDdz );	//��Ϣת�������������ν�Botzone������Ddzƽ̨����
CardCombo lastValidCombo; //��һλ�Ϸ���ҵ�����

//����ͳ��
void setOthersCards( Ddz * pDdz ){

	//���¼�����1
	for(int i=0;i<othersCards.size();++i){
		othersCards[i] = 0;
	}
	bool allcards[54];
	for(int i = 0;i<54;++i){
		allcards[i]=true;//�������Ʊ��Ϊ����
	}
	//�����е���ʷ���Ƶ�������Ϊ�Ѿ�������
	cardRemaining[Landlord] = 20;//Landlord
	cardRemaining[FarmerA] = 17;//FarmerA
	cardRemaining[FarmerB] = 17;//FarmerB
	for(int turn=0; pDdz->iOnTable[turn][0]!=-2; turn++){
		for(int i=0;pDdz->iOnTable[turn][i]!=-1;i++){
			allcards[ pDdz->iOnTable[turn][i] ] = false;//����������������Ϊ�Ѿ���������
			cardRemaining[turn%3] -= 1;//��Ӧ���ݵ��˵�������Ӧ����
		}
	}


	//���Լ����ﻹ�е��Ʊ��Ϊ��֪���ơ�
	//�������Ʊ��Ϊ��֪���ơ�
	for(int i=0;i<3;i++) allcards[pDdz->iLef[i]] = false;
	for(int i=0;pDdz->iOnHand[i]!=-1;i++) allcards[pDdz->iOnHand[i]] = false;
	//����ʣ�µ�Ϊtrue���ƾ���û�е����ˡ�
	for(Card i=0;i<54;++i){
		if(allcards[i]){
			othersCards[card2level(i)]++;
			remainCards[i] = 1;
		}
	}
	initOpponentModel(pDdz);
}

//��Ϣת�������������ν�Botzone������Ddzƽ̨����
void infoConvert( Ddz * pDdz ){
	setOthersCards( pDdz ); //������������ͳ��
	SortById(pDdz->iOnHand);//��Ϣת��infoConvert
	DirConvertPara = (int)( pDdz->cLandlord - 'A' );//λ�ý�������
	switch((int)(pDdz->cDir-pDdz->cLandlord))
	{
	    case 0:myPosition=0;break;
        case 1:myPosition=1;break;
        case 2:myPosition=2;break;
        case -1:myPosition=2;break;
        case -2:myPosition=1;break;
	}
	//myPosition = abs((int)( pDdz->cDir - 'A' ) - DirConvertPara) % 3;//�ҵ����ݣ�FarmerAB or Landlord��
	lastPlayer = ( myPosition + 2 - pDdz->iLastPassCount ) % 3; //�������˵��������ݣ�FarmerAB or Landlord��
	lastTurn = -1;
	for(int i=pDdz->iOTmax;i>=0;i--){
		if(pDdz->iOnTable[i][0] >= 0){
			lastTurn = i;
			break;
		}
	}
	lastValidCombo = CardCombo(); //��λ�ϸ���������ʷ��¼���ڵ�����
	if(lastTurn >= 0){ //��ǰ��
		vector<Card> lastCards;
		for(int i=0;pDdz->iOnTable[lastTurn][i]>=0;i++){
			lastCards.push_back(pDdz->iOnTable[lastTurn][i]);
		}
		lastValidCombo = CardCombo( lastCards.begin(), lastCards.end() );
	}
	rebuildOpponentModel(pDdz);
}

//�����Ƴ�������vector��HelpTakeOff
void removeCards(vector<Card>& ownCards, vector<Card>& deleteCards, vector<Card>& remainCards){
	bool copyFlag;
	remainCards.clear();
	for(Card c: ownCards){
		copyFlag = true;
		for(Card del: deleteCards){
			if(del == c){
				copyFlag = false;
				break;
			}
		}
		if(copyFlag) remainCards.push_back(c);
	}
}
template <typename CARD_ITERATOR>
CardCombo::CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
{
	// ���У���
	if (begin == end)
	{
		comboType = CardComboType::PASS;
		return;
	}

	// ÿ�����ж��ٸ�
	short counts[MAX_LEVEL + 1] = {};

	// ͬ���Ƶ��������ж��ٸ����š����ӡ�������������
	short countOfCount[5] = {};

	cards = vector<Card>(begin, end);
	for (Card c : cards)//��Ͱ�Ż�ԭ����
		counts[card2level(c)]++;
	for (Level l = 0; l <= MAX_LEVEL; l++)
		if (counts[l])
		{
			packs.push_back(CardPack{ l, counts[l] });//��С���󣬱���(0��3��,2)(1��2��,1)(3��5��,3)...
			countOfCount[counts[l]]++;
		}
	sort(packs.begin(), packs.end());//�Ӵ�С

	// ���������������ǿ��ԱȽϴ�С��
	comboLevel = packs[0].level;

	// ��������
	// ���� ͬ���Ƶ����� �м��� ���з���
	vector<int> kindOfCountOfCount;
	for (int i = 0; i <= 4; i++)
		if (countOfCount[i])
			kindOfCountOfCount.push_back(i);
	sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

	int curr, lesser;

	switch (kindOfCountOfCount.size())
	{
	case 1: // ֻ��һ����
		curr = countOfCount[kindOfCountOfCount[0]];
		switch (kindOfCountOfCount[0])
		{
		case 1:
			// ֻ�����ɵ���
			if (curr == 1)
			{
				comboType = CardComboType::SINGLE;
				return;
			}
			if (curr == 2 && packs[1].level == level_joker)
			{
				comboType = CardComboType::ROCKET;
				return;
			}
			if (curr >= 5 && findMaxSeq() == curr &&
				packs.begin()->level <= MAX_STRAIGHT_LEVEL)
			{
				comboType = CardComboType::STRAIGHT;
				return;
			}
			break;
		case 2:
			// ֻ�����ɶ���
			if (curr == 1)
			{
				comboType = CardComboType::PAIR;
				return;
			}
			if (curr >= 3 && findMaxSeq() == curr &&
				packs.begin()->level <= MAX_STRAIGHT_LEVEL)
			{
				comboType = CardComboType::STRAIGHT2;
				return;
			}
			break;
		case 3:
			// ֻ����������
			if (curr == 1)
			{
				comboType = CardComboType::TRIPLET;
				return;
			}
			if (findMaxSeq() == curr &&
				packs.begin()->level <= MAX_STRAIGHT_LEVEL)
			{
				comboType = CardComboType::PLANE;
				return;
			}
			break;
		case 4:
			// ֻ����������
			if (curr == 1)
			{
				comboType = CardComboType::BOMB;
				return;
			}
			if (findMaxSeq() == curr &&
				packs.begin()->level <= MAX_STRAIGHT_LEVEL)
			{
				comboType = CardComboType::SSHUTTLE;
				return;
			}
		}
		break;
	case 2: // ��������
		curr = countOfCount[kindOfCountOfCount[1]];
		lesser = countOfCount[kindOfCountOfCount[0]];
		if (kindOfCountOfCount[1] == 3)
		{
			// ��������
			if (kindOfCountOfCount[0] == 1)
			{
				// ����һ
				if (curr == 1 && lesser == 1)
				{
					comboType = CardComboType::TRIPLET1;
					return;
				}
				if (findMaxSeq() == curr && lesser == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::PLANE1;
					return;
				}
			}
			if (kindOfCountOfCount[0] == 2)
			{
				// ������
				if (curr == 1 && lesser == 1)
				{
					comboType = CardComboType::TRIPLET2;
					return;
				}
				if (findMaxSeq() == curr && lesser == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::PLANE2;
					return;
				}
			}
		}
		if (kindOfCountOfCount[1] == 4)
		{
			// ��������
			if (kindOfCountOfCount[0] == 1)
			{
				// ��������ֻ * n
				if (curr == 1 && lesser == 2)
				{
					comboType = CardComboType::QUADRUPLE2;
					return;
				}
				if (findMaxSeq() == curr && lesser == curr * 2 &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::SSHUTTLE2;
					return;
				}
			}
			if (kindOfCountOfCount[0] == 2)
			{
				// ���������� * n
				if (curr == 1 && lesser == 2)
				{
					comboType = CardComboType::QUADRUPLE4;
					return;
				}
				if (findMaxSeq() == curr && lesser == curr * 2 &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::SSHUTTLE4;
					return;
				}
			}
		}
	}
	comboType = CardComboType::INVALID;
}

/********LIAO**********/
float CardCombo::getValue2() const//������Ȩֵ�Ż�
{
	Level value = this->comboLevel;
    bool biggestS = true;//�ж�����Ƿ�Ϊ����
	bool biggestP = true;
	bool biggestT = true;						//�ж��Ƿ�Ϊ���ԡ�������
	bool biggestD = true;
	bool DoubleKing = false;//�ж��Ƿ���˫��
	bool FourBoom = false;//�ж��Ƿ���ը��

    if((othersCards[13]+othersCards[14])>=2){DoubleKing=true; }//�ж�˫���Ƿ��п��ܴ����������ƣ�����У�����Ӧֵ��Ϊtrue

    for(Level l=0;l<13;++l)//�ж��Ƿ���ը��������������
    {
        if(othersCards[l]>3)FourBoom=true;
    }

/***************�ȴ�����һ���жϣ�ʹ���ڹؼ�ʱ��׼ȷ�ж�һ�������Ƿ��ǿ���******************/
	for(Level l=comboLevel+1;l<MAX_LEVEL;++l){	//�ж��Ƿ�Ϊ�����
		if(othersCards[l] or (DoubleKing==true) or (FourBoom==true)) biggestS = false;
	}

	for(Level l=comboLevel+1;l<13;++l){
		if(othersCards[l]>1 or (DoubleKing==true) or (FourBoom==true)) biggestP = false;//�ж϶���
		if(othersCards[l]>2 or (DoubleKing==true) or (FourBoom==true)) biggestT = false;//�ж�3
		if(othersCards[l]>3 or (DoubleKing==true) or (FourBoom==true)) biggestD = false;//�ж�4
	}

	if(comboLevel==12){
        if((DoubleKing==true) or (FourBoom==true)){
                biggestP = false;
                biggestT = false;
                biggestD = false;
        }
	}


	bool biggestST  = true;
	bool biggestST2 = true;
	bool biggestPL  = true;
	bool biggestBOOM = true;
	for(Level l=comboLevel+1;l<12;++l){			//�ж��Ƿ�Ϊ���˳��
		if(othersCards[l] or (DoubleKing==true) or (FourBoom==true)) biggestST = false;
		if(othersCards[l]>1 or (DoubleKing==true) or (FourBoom==true)) biggestST2 = false;
		if(othersCards[l]>2 or (DoubleKing==true) or (FourBoom==true)) biggestPL  = false;

	}
    for(Level l=comboLevel+1;l<13;++l){
        if(othersCards[l]>3 or (DoubleKing==true)) biggestBOOM  = false;
    }
	if(!biggestS  && isLikelyBiggest(comboLevel, 1, 301)) biggestS = true;
	if(!biggestP  && isLikelyBiggest(comboLevel, 2, 402)) biggestP = true;
	if(!biggestT  && isLikelyBiggest(comboLevel, 3, 503)) biggestT = true;
	//��֪�Լ����Ƶ���Ϣ��ʣ���Ƶ���Ϣ����Ȩ��ֵ����һЩ�ı�
	if(comboType==CardComboType::SINGLE){
		if(biggestS){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value-roundValWithDecay;
	}
	else if(comboType==CardComboType::PAIR){
        int addValueNumber=0;
        //if(value>=10)addValueNumber=2;
		if(biggestP){
			value = roundValWithDecay;
		}
		else{
		value = baseCardsWeight * value-roundValWithDecay+addValueNumber;
		}
	}
	else if(comboType==CardComboType::TRIPLET){
        int addValueNumber=0;
        //if(value>=11)addValueNumber=2;
		if(biggestT){
			value = roundValWithDecay;
		}
		else{
		value = baseCardsWeight * value-roundValWithDecay+addValueNumber;
		}
	}
	else if(comboType==CardComboType::TRIPLET1){
        int addValueNumber=0;
        //if(value>=11)addValueNumber=2;
		if(biggestT){
			value = roundValWithDecay;
		}
		else{
		value = baseCardsWeight * value-roundValWithDecay+addValueNumber;
		}
	}
	else if(comboType==CardComboType::TRIPLET2){
	    int addValueNumber=0;
        //if(value>=11)addValueNumber=2;
		if(biggestT){
			value = roundValWithDecay;
		}
		else{
		value = baseCardsWeight * value-roundValWithDecay+addValueNumber;

		}

	}
	else if(comboType==CardComboType::STRAIGHT){
		if(biggestST){
			value = roundValWithDecay;
		}
		else
        {
		value = baseCardsWeight * value-roundValWithDecay;

		}
	}
	else if(comboType==CardComboType::STRAIGHT2){
		if(biggestST2){
			value = roundValWithDecay;
		}
		else
		{
		value = baseCardsWeight * value-roundValWithDecay;

		}
	}
	else if(comboType==CardComboType::BOMB){
        if(biggestBOOM)
		value = baseCardsWeight * value + roundValWithDecay;
		else value=6;
	}
	else if(comboType==CardComboType::ROCKET){//��ը
		value = RocketVal;
	}
	else if(comboType==CardComboType::QUADRUPLE2){
		if(biggestD){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value/2;
	}
	else if(comboType==CardComboType::QUADRUPLE4){
		if(biggestD){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value/2;
	}
	else if(comboType==CardComboType::PLANE){
		if(biggestPL){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value/2;
	}
	else if(comboType==CardComboType::PLANE1){
		if(biggestPL){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value/2;
	}
	else if(comboType==CardComboType::PLANE2){
		if(biggestPL){
			value = roundValWithDecay;
		}
		else
		value = baseCardsWeight * value/2;
	}
	else value = 0;
	return value;
}
float CardCombo::getSeqValue(comboDiv& ComboSeq){ //��ȡ������value
	float total = 0;
	for(comboDiv::iterator x = ComboSeq.begin(); x!=ComboSeq.end(); x++){
		CardCombo part = *x;
		total += part.getValue2();
	}
	return SeqDecayRate * total;
}

/*---------------�����ظ������ĺ���---------------*/
float CardCombo::MergeValue(CardCombo main, comboDiv DivRemain)//�������Ĵ��ɻ��Լ�����ɻ��ĺϲ�
{
	comboDiv singles, pairs;//�������浥����
	//short count;//���ڼ���x��n������n
	float Value1=1000, Value2=1000;//���ڼ���������˫��ֵ
	for(comboDiv::iterator k = DivRemain.begin(); k!= DivRemain.end(); k++){//ͳ�Ƶ��ͶԶ�������
		CardCombo part = *k;
		if(part.comboType == CardComboType::SINGLE){
			singles.push_back(part);
		}
		if(part.comboType == CardComboType::PAIR){
			pairs.push_back(part);
		}
	}
	sort(singles.begin(),singles.end());
	sort(pairs.begin(),pairs.end());
	float ComboValue = 0;
	if(main.comboType == CardComboType::SSHUTTLE){//����ɻ�
		Level mainlv = main.comboLevel;//����Level
		Level dlv = main.packs.size(); //�������г���
		if((short)singles.size() >= 2*dlv) {//��2n����
			Value1 = getExValue(singles, mainlv, dlv, 2);
		}
		if((short)pairs.size() >= 2*dlv) {//��2n����
			Value2 = getExValue(pairs, mainlv, dlv, 2);
		}
		ComboValue = 4 * roundValWithDecay - minimum(Value1,Value2); //�����ĸ���ϣ������ĸ��غ�
	}
	if(main.comboType == CardComboType::PLANE){//�ɻ�
		Level mainlv = main.comboLevel;//����Level
		Level dlv = main.packs.size(); //�������г���
		if((short)singles.size() >= dlv) {//��n����
			Value1 = getExValue(singles, mainlv, dlv, 1);
		}
		if((short)pairs.size() >= dlv) {//��n����
			Value2 = getExValue(pairs, mainlv, dlv, 1);
		}
		ComboValue = 2 * roundValWithDecay - minimum(Value1,Value2);//���������������������ػغ�
	}
	if(main.comboType == CardComboType::BOMB){//�Ĵ�
		Level mainlv = main.comboLevel;//����Level
		if((short)singles.size() >= 2) {//��2����
			Value1 = getExValue(singles, mainlv, 1, 2);
		}
		if((short)pairs.size() >= 2) {//��2����
			Value2 = getExValue(pairs, mainlv, 1, 2);
		}
		ComboValue = 2 * roundValWithDecay - minimum(Value1,Value2) + mainlv/2 - main.getValue2(); //����������ϣ����������غ� ��û��һ��ը��������һ���Ĵ���
	}
	if(main.comboType == CardComboType::TRIPLET){//����
		Level mainlv = main.comboLevel;//����Level
		if((short)singles.size() >= 1) {//��1����
			Value1 = getExValue(singles, mainlv, 1, 1);
		}
		if((short)pairs.size() >= 1) {//��1����
			Value2 = getExValue(pairs, mainlv, 1, 1);
		}
		ComboValue = roundValWithDecay - minimum(Value1,Value2);//����1����������1���غ�
	}
	ComboValue = maximum( 0 , ComboValue); // ����������ȥ
	return ComboValue;
}

float getComboPenalty(const comboDiv& divSeq) {
	float penalty = 0;
	for (const CardCombo& combo : divSeq) {
		switch (combo.comboType) {
			case CardComboType::SINGLE:    penalty += 9;  break;
			case CardComboType::PAIR:      penalty += 7;  break;
			case CardComboType::TRIPLET:   case CardComboType::TRIPLET1:
			case CardComboType::TRIPLET2:  penalty += 6;  break;
			case CardComboType::STRAIGHT:  penalty += 6;  break;
			case CardComboType::STRAIGHT2: penalty += 5;  break;
			case CardComboType::PLANE:     case CardComboType::PLANE1:
			case CardComboType::PLANE2:    penalty += 5;  break;
			case CardComboType::BOMB:      penalty += 3;  break;
			case CardComboType::ROCKET:    penalty += 1;  break;
			default:                       penalty += 7;  break;
		}
	}
	return penalty;
}

void CardCombo::ValueCompare(vector<Card>& divcards,vector<Card>& cuts, comboDiv& DivRemain, float& MaxValue){
	ComboDiv TempSeq = div( CardCombo( divcards.begin(),divcards.end() ) ); //�нӷָ�����������
	CardCombo CutCombo = CardCombo( cuts.begin(),cuts.end() );
	float ComboValue = TempSeq.Value + CutCombo.getValue2() ;//������е�Ȩֵ
	if( CutCombo.comboType == CardComboType::TRIPLET or
		CutCombo.comboType == CardComboType::PLANE or
		CutCombo.comboType == CardComboType::SSHUTTLE or
		CutCombo.comboType == CardComboType::BOMB
		){
		ComboValue += MergeValue(CutCombo, TempSeq.div);
	}
	float penalty1 = getComboPenalty(TempSeq.div) + 7;
	float penalty2 = getComboPenalty(DivRemain);
	if(ComboValue - penalty1 > MaxValue - penalty2){
		MaxValue = ComboValue;
		DivRemain.assign(TempSeq.div.begin(),TempSeq.div.end());
		DivRemain.push_back( CutCombo );//��cut�������������������
	}
}
//�������Ʋ��ֵ�����Ȩֵ�ȽϺ���������ֵ�����Ƿ�����myAction
bool CardCombo::RemainValueCompare(vector<Card>& remainCards, ComboDiv& FinalSeq, CardCombo& myAction){
	CardCombo remainCombo = CardCombo( remainCards.begin(), remainCards.end());
	ComboDiv tempSeq = remainCombo.div( remainCombo ); //������������
	float penalty1 = getComboPenalty(tempSeq.div);
	float penalty2 = getComboPenalty(FinalSeq.div);
	if(tempSeq.Value - penalty1 > FinalSeq.Value - penalty2){//�Ƚ��뵱ǰ���ֵ������Ȩֵ��С
		FinalSeq = tempSeq;//ȡ���
		return true;//��TRUE�������������myAction
	}
	else return false;
}

float CardCombo::getExValue(comboDiv& Seq, short mainlv, short dlv, short k){
	short count = 0, Value = 0;
	for(auto c = Seq.begin(); c!= Seq.end(); c++){
		Level tmplv = c->comboLevel;
		if(tmplv<=mainlv-dlv or tmplv>mainlv){
			count++;
			Value += c->getValue2();//����ط���ʵӦ�ÿ��ǻ���getValue2
		}
		if(count >= k*dlv) break;
	}
	if(count < k*dlv) Value = 1000;//����
	return Value;
}

CardCombo CardCombo::MergeCombo(comboDiv& singles, comboDiv& pairs)//�������Ĵ��ɻ��Լ�����ɻ��ĺϲ�
{
	short SingleValue1 = 100, SingleValue2 = 100, PairValue1 = 100, PairValue2 = 100;//���ڼ���������˫��ֵ
	if(singles.size() >= 1){
		SingleValue1 = (singles.begin())->comboLevel;//��һ����
		if(singles.size() >= 2){
			SingleValue2 = SingleValue1 + (singles.begin()+1)->comboLevel;//����������
		}
	}
	if(pairs.size() >= 1){
		PairValue1 = (pairs.begin())->comboLevel;
		if(pairs.size() >= 2){
			PairValue2 = PairValue1 + (pairs.begin()+1)->comboLevel;//�������Զ�
		}
	}
	int len = this->packs.size();
	CardCombo tempAction = *this;
	if(this->comboType == CardComboType::PLANE and (PairValue2!=100 or SingleValue2!=100)){//�ɻ� is Coming
		if(SingleValue2 < PairValue2 and singles.size()>=len){//�ɻ���С��
			CardCombo part[5];
			for(int j=0;j<len;j++) part[j] = *(singles.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<len;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=len){//�ɻ�������
			CardCombo part[5];
			for(int j=0;j<len;j++) part[j] = *(pairs.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<len;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
	}
	if(this->comboType==CardComboType::TRIPLET){//����
		if(SingleValue1 < PairValue1){//����һ
			CardCombo part = *singles.begin();
			vector<Card> ActionCards = this->cards;
			ActionCards.insert( ActionCards.end(), part.cards.begin(), part.cards.end() );
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=1){//����һ
			CardCombo part = *pairs.begin();
			vector<Card> ActionCards = this->cards;
			ActionCards.insert( ActionCards.end(), part.cards.begin(), part.cards.end() );
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
	}
	if(this->comboType==CardComboType::BOMB){//�Ĵ�
		if(SingleValue2 < PairValue2){//�Ĵ�һ
			CardCombo part[5];
			for(int j=0;j<2;j++) part[j] = *(singles.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<2;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=2){//�Ĵ���
			CardCombo part[5];
			for(int j=0;j<2;j++) part[j] = *(pairs.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<2;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
	}
	return tempAction;
}

/*---------------�����ظ������ĺ���---------------*/

/**
 * ����������CardPack�ݼ��˼���
 * ���ڼ���˳��ʲô��
 */
int CardCombo::findMaxSeq() const
{
	for (unsigned c = 1; c < packs.size(); c++)
		if (packs[c].count != packs[0].count ||
			packs[c].level != packs[c - 1].level - 1)
			return c;
	return packs.size();
}

Seq CardCombo::findSeq1() const //��ⵥ˳
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//��Ͱ�Ż�ԭ����
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=4; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 1)
				break;
		}
		len = start - end;
		if(len >= 5) return Seq{start, len};//0,1,2,3,4��5����
	}
	return Seq{12,1};//����
}
Seq CardCombo::findSeq2() const //���˫˳
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//��Ͱ�Ż�ԭ����
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=2; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 2)
				break;
		}
		len = start - end;
		if(len >= 3) return Seq{start, len};
	}
	return Seq{12,1};//����
}
Seq CardCombo::findSeq3() const //���ɻ�
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//��Ͱ�Ż�ԭ����
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=1; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 3)
				break;
		}
		len = start - end;
		if(len >= 2) {return Seq{start, len};}
	}
	return Seq{12,1};//����
}
Seq CardCombo::findSeq4() const //��⺽��ɻ�
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//��Ͱ�Ż�ԭ����
		counts[card2level(c)]++;
	//if(counts[MAX_STRAIGHT_LEVEL+1] == 4) return Seq{MAX_STRAIGHT_LEVEL+1, 1};
	for(start = MAX_STRAIGHT_LEVEL; start>=0; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 4)
				break;
		}
		len = start - end;
		if(len >= 1) return Seq{start, len};
	}
	return Seq{12,1};//����
}
/*�ж�ָ�������ܷ�����ǰ���飨������������ǹ��Ƶ��������*/
bool CardCombo::canBeBeatenBy(const CardCombo& b) const{
	if (comboType == CardComboType::INVALID || b.comboType == CardComboType::INVALID)
		return false;
	if (b.comboType == CardComboType::ROCKET)
		return true;
	if (b.comboType == CardComboType::BOMB)
		switch (comboType)
		{
		case CardComboType::ROCKET:
			return false;
		case CardComboType::BOMB:
			return b.comboLevel > comboLevel;
		default:
			return true;
		}
	return b.comboType == comboType && b.cards.size() == cards.size() && b.comboLevel > comboLevel;
}

//�������ƺ���
CardCombo HelpPass(struct Ddz * pDdz)
{
    //dout<<"HelpPass"<<endl;
	vector<Card> myCards;
	for(int i=0;pDdz->iOnHand[i]!=-1;i++){
		myCards.push_back(pDdz->iOnHand[i]);
	}

	//�����ƽ��д���
	CardCombo myCardCombo(myCards.begin(),myCards.end());
	ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
	comboDiv myCardsDiv = myComboDiv.div;

	sort(myCardsDiv.begin(),myCardsDiv.end());

	vector<Card> remainCards;
	CardCombo myAction,tempAction,remainCombo,smallSingle,smallPair;
	ComboDiv tempSeq,FinalSeq;
	comboDiv singles, pairs,straight,plane;//�������浥����
	comboDiv normal,king;	// normal �ǿ��֣� king ����
	comboDiv singleCombo,otherCombo,pairCombo;
	float SingleValue1 = 100, SingleValue2 = 100, PairValue1 = 100, PairValue2 = 100;//���ڼ���������˫��ֵ
	for(vector<CardCombo>::iterator k = myCardsDiv.begin(); k!= myCardsDiv.end(); k++){//ͳ�Ƶ��ͶԶ�������
		CardCombo part = *k;
		if(part.getValue2()>=roundValWithDecay)
			king.push_back(part);
		else normal.push_back(part);

		if(part.comboType==CardComboType::SINGLE)
			singleCombo.push_back(part);
		else {
                if(part.comboType==CardComboType::PAIR)pairCombo.push_back(part);
                otherCombo.push_back(part);
        }

        if(part.comboType == CardComboType::SINGLE)
			singles.push_back(part);
		if(part.comboType == CardComboType::PAIR)
			pairs.push_back(part);
        if(part.comboType == CardComboType::STRAIGHT or part.comboType == CardComboType::STRAIGHT2)
            straight.push_back(part);
        if(part.comboType == CardComboType::PLANE or part.comboType == CardComboType::PLANE1 or part.comboType == CardComboType::PLANE2)
			plane.push_back(part);

	}
	sort(myCards.begin(),myCards.end());
	sort(straight.begin(),straight.end());
	sort(singles.begin(),singles.end());
	sort(pairs.begin(),pairs.end());
	sort(singleCombo.begin(),singleCombo.end());
	sort(otherCombo.begin(),otherCombo.end());

    if(singleCombo.size()>0)
        smallSingle=*(singleCombo.begin());
    if(pairCombo.size()>0)
        smallPair=*(pairCombo.begin());

	if(singles.size() >= 1){
        //dout<<"singles"<<endl;
		SingleValue1 = (singles.begin())->comboLevel;//��һ����
		if(singles.size() >= 2){
			SingleValue2 = SingleValue1 + (singles.begin()+1)->comboLevel;//����������
		}
	}
	if(pairs.size() >= 1){
		PairValue1 = (pairs.begin())->comboLevel;
		if(pairs.size() >= 2){
			PairValue2 = PairValue1 + (pairs.begin()+1)->comboLevel;//�������Զ�
		}
	}

	//��Ӯ�о��о�
	int num_normals = normal.size();
	int kicker_remain = singles.size() + pairs.size();
	for(int j = 0; j < king.size(); j++){
		if(king[j].comboType == CardComboType::SINGLE || king[j].comboType == CardComboType::PAIR){
			kicker_remain -= 1;					//�ǿ�������ĵ��ƺͶ����м���
		}
	}
	int accumulate = 0;
	for(vector<CardCombo>::iterator part = myCardsDiv.begin(); part!= myCardsDiv.end(); part++){
		if(part->comboType == CardComboType::TRIPLET){
			if(kicker_remain > 0){
				accumulate += 1;				// �������Ե���һ�ţ����ۼ���������ټ���
			}
		}
		if(part->comboType == CardComboType::PLANE){
			if(kicker_remain > 1){
				num_normals -= 2;
				kicker_remain -= 2;
			}
		}
		if(part->comboType == CardComboType::BOMB){
			if(kicker_remain > 1){
				num_normals -= 2;
				kicker_remain -= 2;
			}
		}
	}
	num_normals = num_normals - min(kicker_remain, accumulate);		// ��������㹻�ͼ�������������������ֻ�ܰѼ������д��Ƶ�����

/********LIAO**********/
        if(myCardCombo.comboType != CardComboType::INVALID){ //��һ���ƾ�ֱ�ӳ�����
			if(myCardCombo.packs.size() > 1 && myCardCombo.packs[0].count == 3 && myCardCombo.packs[1].count != 3){
				if(myCardCombo.packs.size() > 2 && (myCardCombo.packs)[1].level == 14 && (myCardCombo.packs)[2].level == 13){			//	�����ǲ�����ը
					vector<Card> rockets = {card_joker, card_JOKER};
					CardCombo RocketCombo = CardCombo(rockets.begin(), rockets.end());
					return RocketCombo;
				}
			}
		   return myCardCombo;
		}
        if(num_normals<=1 && king.size()>0){//���ֻ��һ�ŷǿ��֣�һֱ������
            //dout<<"KingOut"<<endl;
            sort(king.begin(),king.end());
            CardCombo tempAction = *(king.begin());
            myAction = tempAction.MergeCombo(singles,pairs);
            return myAction;
        }

//�������������˳�ӡ��ɻ��Ĵ�С����û�п��֣������ȳ�

        if(straight.size()>0){
                int small_6_straight=0;
                CardCombo Support;
                Support=*(straight.begin());
                if(Support.comboLevel<=8)small_6_straight=1;

                if(myPosition==FarmerA||myPosition==FarmerB){

                        if(cardRemaining[Landlord]<Support.packs.size() or Support.getValue2()>=7 or small_6_straight){
                                return myAction = *(straight.begin());
                        }
                }
        if(myPosition==Landlord){
                        if((cardRemaining[FarmerA]<Support.packs.size() and cardRemaining[FarmerB]<Support.packs.size() )or Support.getValue2()>=7 or small_6_straight){
                                return myAction = *(straight.begin());
                        }
                }
        }

        if(plane.size()>0){
                CardCombo Support;
                Support=*(plane.begin());
                Support=Support.MergeCombo(singles,pairs);

                if(myPosition==FarmerA||myPosition==FarmerB){
                        if(cardRemaining[Landlord]<Support.packs.size() or Support.getValue2()>=7){
                                return myAction = Support;
                        }
                }
                if(myPosition==Landlord){
                         if((cardRemaining[FarmerA]<Support.packs.size() and cardRemaining[FarmerB]<Support.packs.size()) or Support.getValue2()>=7){
                                return myAction = Support;
                        }
                }
        }
        //if(straight.size()>0){return myAction = *( straight.begin() );}

/********LIAO**********/
	// edited Liao
	{//����/����ֻʣһ���Ƶı��زоֲ���
		if(myPosition == Landlord){//����
		    //dout<<"Landlord:"<<Landlord<<endl;
			if(cardRemaining[FarmerA]==1 ){//���ũ���ֻʣһ����
			    //dout << "LandlordToFarmerA"<<endl;
				/********����ֻ��ը�������********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//������ֻ��һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(otherCombo.begin());//�Ȱ�ը������
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//�����ƶ���һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(singleCombo.rbegin());//�Ȱѵ��Ƶ��ų�
					}

                /********���������********/
                else {
                        //dout<<"LandlordToFarmerA OtherSituation"<<endl;
                        if(otherCombo.size()>0){
                            //dout<<"LandlordToFarmerA OtherSituation1"<<endl;
                            CardCombo tempAction = *( otherCombo.begin() );
                            return myAction = tempAction.MergeCombo(singles,pairs);

                        }
                        else
                        {
                            return myAction = *(singleCombo.rbegin());
                        }
                }

			}
			if(cardRemaining[FarmerB]==1 ){//���ũ����ֻʣһ����
				/********����ֻ��ը�������********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//������ֻ��һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(otherCombo.begin());//�Ȱ�ը������
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//�����ƶ���һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(singleCombo.rbegin());//�Ȱѵ��Ƶ��ų�
					}
                /********���������********/
                 /*if(singleCombo.begin()->comboLevel>11 and otherCombo.size()==1)//���ʣ�µĵ��ƺܴ󣬵��Ƿǵ���ֻ��һ������ʵӦ�����жϷǵ��ż�ֵ)���ȳ�����
                        return myAction = *(singleCombo.rbegin());*/
                else {

                            if(otherCombo.size()>0)
                            {
                            //dout<<"LandlordToFarmerB OtherSituation otherCombo"<<endl;
                            CardCombo tempAction = *( otherCombo.begin() );
                            return myAction = tempAction.MergeCombo(singles,pairs);
                            }
                            else
                            {
                            //dout<<"LandlordToFarmerB OtherSituation singleCombo"<<endl;

                            return myAction= *( singleCombo.rbegin() );
                            }
                }
			}

		}
		else if(myPosition == FarmerA){//ũ���
           // dout<<"FarmerA:"<<FarmerA<<endl;
			if(cardRemaining[FarmerB] == 1 && AtoB_dedication==0){//����¼�FarmerBֻ��һ���ƣ��Ͻ�����Ӯ

			    CardCombo Support;
			    Support=*(singleCombo.begin());
				if(singleCombo.size()>0 and Support.getValue2()<7 and Support.comboLevel<11)
                {//��9���µĵ���
                    AtoB_dedication=1;
					return *singleCombo.begin(); //��С���Ƴ�С�������ƣ�
                }
				else if(card2level(*myCards.begin())<6)
                    {//��9���µ��ƣ�����Լ���comboҲҪ����
                        AtoB_dedication=1;
                        return CardCombo(myCards.begin(),++myCards.begin());
					}
			}

			else if(cardRemaining[Landlord] == 1){//�������ֻʣһ����
                //dout<<"FAToFB"<<endl;
				/********����ֻ��ը�������********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//������ֻ��һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(otherCombo.begin());//�Ȱ�ը������
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//�����ƶ���һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(singleCombo.rbegin());//�Ȱѵ��Ƶ��ų�
					}

                /********���������********/
                else {
                            if(otherCombo.size()>0)
                            {
                                CardCombo tempAction = *( otherCombo.begin() );
                                return myAction = tempAction.MergeCombo(singles,pairs);
                            }
                            else
                            {
                            return myAction= *( singleCombo.rbegin());
                            }
                }
			}
		}
		else if(myPosition == FarmerB){//ũ����
			if(cardRemaining[Landlord] == 1){//�������ֻʣһ����

			    //dout << "FarmerBToLandlord"<<endl;
				/********����ֻ��ը�������********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//������ֻ��һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(otherCombo.begin());//�Ȱ�ը������
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//�����ƶ���һ���ҷǵ�����һ��ը��
					{
                        return myAction = *(singleCombo.rbegin());//�Ȱѵ��Ƶ��ų�
					}

                /********���������********/
                if(otherCombo.size()>0)
                {
                    CardCombo tempAction = *( otherCombo.begin() );
                    return myAction = tempAction.MergeCombo(singles,pairs);
                }
                else{
                        return myAction = *(singleCombo.rbegin());

                }
			}
		}

	}

	/* Endgame: big single (2/joker/JOKER) + few small singles, play small first. */
	if(king.size() == 1 && singles.size() >= 2 && singles.size() <= 3) {
		CardCombo kingCard = king[0];
		if(kingCard.comboType == CardComboType::SINGLE && kingCard.comboLevel >= 12) {
			for(auto it = singles.rbegin(); it != singles.rend(); ++it)
				if(it->comboLevel < 12) return *it;
			return *(singles.rbegin());
		}
	}

	/* Universal principle: when in control, play smallest combo first.
	 * Small cards are harder to play later (opponents play bigger).
	 * Save big cards for critical moments. */
	if(pDdz->iLastTypeCount == 0 && myCardsDiv.size() > 1) {
		/* Find the combo with the smallest level, prefer it */
		CardCombo smallestCombo = *(myCardsDiv.rbegin()); // rbegin=smallest
		/* Only apply if it's a reasonable play (not passing up a winning move) */
		if(smallestCombo.comboLevel <= 10) { // Don't force on big combos
			CardCombo tempAction = smallestCombo.MergeCombo(singles,pairs);
			return tempAction;
		}
	}

	{ //---------------------Ȩֵ������������---------------------
		{ //---------------------Ȩֵ������������---------------------
		FinalSeq = myCardCombo.div( myCardCombo );//myActionΪPass�������ƶ������ƣ�����ʼ��
		FinalSeq.Value = -1000;

		for(auto combos = myCardsDiv.begin() ; combos != myCardsDiv.end() ; combos++){
			int len = combos->packs.size();
			CardCombo main = *combos;
			if(	main.comboType != CardComboType::SINGLE and main.comboType != CardComboType::PAIR)
				tempAction = main.MergeCombo(singles,pairs);
			removeCards(myCardCombo.cards,tempAction.cards,remainCards);
			remainCombo = CardCombo(remainCards.begin(), remainCards.end());
			if( myCardCombo.RemainValueCompare(remainCards, FinalSeq, myAction) ){
				myAction = tempAction;
			}
		}
		//��������SINGLE&PAIR����
		if(singles.size()==1 and pairs.size()>1){//ֻ��һ�����Ƶ�ʱ��PAIR�࿼���ȳ��Զ��������ܵ��ϣ�
			tempAction = *(pairs.begin());
		}
		else if(pairs.size()==1 and singles.size()>1){//ֻ��һ���Զ���ʱ��SINGLE�࿼���ȳ����������ܵ��ϣ�
			tempAction = *(singles.begin());
		}
		else{
			if( SingleValue1 < PairValue1 )
				tempAction = *(singles.begin());
			else if( SingleValue1 > PairValue1 )
				tempAction = *(pairs.begin());
			else tempAction = *(myCardsDiv.begin());
		}
		removeCards(myCardCombo.cards,tempAction.cards,remainCards);
		if( myCardCombo.RemainValueCompare(remainCards, FinalSeq, myAction) ){
			myAction = tempAction;
		}
	}
	}
	string str = print(FinalSeq.div);
	const char* Seqs = str.data();
	FinalSeq.Value -= FinalSeq.div.size()*roundValWithDecay;
	str = "Value="+ toString(FinalSeq.Value);
	const char* Seqss = str.data();

	if(myAction.cards.size() == 0){
		myAction = tempAction;		// HelpPass������ƣ�����ֱ����
	}
	return myAction;
}

void HelpPla(struct Ddz * pDdz){
	for(int i=0;i<kPlaMax;i++)		//��ʼ��
		for(int j=0;j<21;j++) pDdz->iPlaArr[i][j]=-1;
	pDdz->iPlaCount=0; //���ƿ��н�������ʼ��Ϊ0
	if (pDdz->iLastTypeCount==0){
		//��������PASS���������������ģ��
		CardCombo myAction = HelpPass(pDdz); //���������Ե���
		//������Ҫ��myAction.cardsת���������滻iToTable
		int k=0;
		for(auto i=myAction.cards.begin(); i!=myAction.cards.end(); i++){
			pDdz->iPlaArr[pDdz->iPlaCount][k++] = *i;
		}
		pDdz->iPlaArr[pDdz->iPlaCount][k] = -1;
		pDdz->iPlaCount++;		// �������ƿ��н��1
		return;
	}
	else{
		Rocket(pDdz); //���
		Bomb(pDdz); //ը��
		//�������߿���ѹ��ͬ����
		//�������ƣ����ͱ�Ȼ��Ҫѹ���ƶ�Ӧ
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


//D01-START���㵱ǰ�������ƹ�ֵ,Ԥ�費����ƺ����ƣ������һ����������
//����޶���:л��&÷��,����޶�ʱ��:15-02-11
double CalCardsValue(int iPlaOnHand[])
{
	double dSum = -10000;			//��ֵ
	//��Ϣת��infoConvert
		SortById(iPlaOnHand);
		vector<Card> myCards;
		for(int i=0;iPlaOnHand[i]!=-1;i++) myCards.push_back(iPlaOnHand[i]);
		//�����ƽ��д���
		CardCombo myCardCombo(myCards.begin(),myCards.end());
		ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
		comboDiv myCardsDiv = myComboDiv.div;
		sort(myCardsDiv.begin(),myCardsDiv.end());
	dSum = myComboDiv.Value - roundValWithDecay*(short)myCardsDiv.size();
	return dSum;
}
//D01-END
//I02-END
//I02-START���㼺�����Ʋ���:Ԥ��3�ֻ�0�֣������һ����������
//����޶���:÷��,����޶�ʱ��:15-02-12
int CalBid(struct Ddz * pDdz	)
{
	int iMyBid = 0;
	setOthersCards(pDdz);
	int lvC[15] = {0};
	for (int i = 0; pDdz->iOnHand[i] >= 0; i++) lvC[card2level(pDdz->iOnHand[i])]++;
	int c2 = lvC[12], cJo = lvC[13], cJO = lvC[14], bombs = 0;
	for (int lv = 0; lv < 13; lv++) if (lvC[lv] >= 4) bombs++;
	if ((cJo>=1&&cJO>=1) || (cJO>=1&&c2>=2) || (bombs>=1&&c2>=1&&cJO>=1)) iMyBid = 3;
	else if (c2 >= 2 || (cJO >= 1 && c2 >= 1)) iMyBid = 2;
	else if (c2 >= 1 || cJO >= 1) iMyBid = 1;
	for (int i = 0; i < 3; i++) if (pDdz->iBid[i] >= iMyBid) iMyBid = 0;
	return iMyBid;
}

/*
	�����������繹����������
*/
int coordTransform(int card, int z){
	int x = card / 4;
	int y = card % 4;
	return dim1 * z + dim2 * x + y;
}

int coordLevelTransform(int cardLevel, int z, int code){
	int axis = dim1 * z + 60 + dim2 * cardLevel + code;
	return axis;
}

void checkSeq(vector<int>& _len, vector<int>& limit, CardCombo localCardCombo,
				int index, short* counts, short* countOfCount, int z){
	Level localLevel = localCardCombo.packs[index].level;
	int localCount = localCardCombo.packs[index].count;
	int coord;
	if(_len[localCount] >= limit[localCount]){			// localLevel ǰ _len ���ȼ����˳��
		if(localCount == 3){	//����˳
			if(countOfCount[2] >= 2){	//���ڶ��ӣ�������ɻ�����
				for(int k = 0; k < _len[localCount]; k++){
					coord = coordLevelTransform(localLevel - k, z, 12);		//�ɻ�λ����1
					feature[coord] = 1;
				}
			}
			if(countOfCount[1] >= 2){	//���ڵ��ƣ�������ɻ�����
				for(int k = 0; k < _len[localCount]; k++){
					coord = coordLevelTransform(localLevel - k, z, 11);		//�ɻ�λ����1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 4);		//��˳λ����1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 3);		//˫˳λ����1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 2);		//����˫˳Ҳ������ɵ�˳�����Ե�˳λ��Ҳ��1
					feature[coord] = 1;
				}
			}
		}
		else if(localCount == 2){	//��˫˳
			int coord;
			for(int k = 0; k < _len[localCount]; k++){
				coord = coordLevelTransform(localLevel - k, z, 3);		//˫˳λ����1
				feature[coord] = 1;
				coord = coordLevelTransform(localLevel - k, z, 2);		//����˫˳Ҳ������ɵ�˳�����Ե�˳λ��Ҳ��1
				feature[coord] = 1;
			}
		}
		else{	//��˳��
			int coord;
			for(int k = 0; k < _len[localCount]; k++){
				coord = coordLevelTransform(localLevel - k, z, 2);
				feature[coord] = 1;
			}
		}
	}
}

void add_feature(vector<int>& cardsInThisTurn, int z, int layer){//������������
	CardCombo localCardCombo = CardCombo(cardsInThisTurn.begin(), cardsInThisTurn.end());
	if(layer == 1){		//������һ����������
		auto &localPack = localCardCombo.packs;
		vector<Card> &localCard = localCardCombo.cards;

		short * counts = new short[MAX_LEVEL + 1];
		short * countOfCount = new short[5];
		memset(counts, 0, (MAX_LEVEL + 1) * sizeof(short));
		memset(countOfCount, 0, 5 * sizeof(short));

		for (Card c : localCard)//��Ͱ�Ż�ԭ����
			counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
				countOfCount[counts[l]]++;

		for(int i = 0; i < localPack.size(); i++){
			auto item = localPack[i];
			int coord;
			if(item.count == 4){
				Level localLevel = item.level;
				if(countOfCount[2] > 0){	//������Ĵ���
					coord = coordLevelTransform(localLevel, z, 8);
					feature[coord] = 1;
				}
				if(countOfCount[1] > 0){	//������Ĵ�һ
					coord = coordLevelTransform(localLevel, z, 7);
					feature[coord] = 1;
				}
				coord = coordLevelTransform(localLevel, z, 6);	//����ը��
				feature[coord] = 1;
			}
			else if(item.count == 3){
				Level localLevel = item.level;
				if(countOfCount[2] > 0){	//��������
					coord = coordLevelTransform(localLevel, z, 10);	//����������
					feature[coord] = 1;
				}
				if(countOfCount[1] > 0){	//������һ
					coord = coordLevelTransform(localLevel, z, 9);	//��������һ
					feature[coord] = 1;
				}
				coord = coordLevelTransform(localLevel, z, 5);	//��������
				feature[coord] = 1;
			}
			else if(item.count == 2){
				Level localLevel = item.level;
				coord = coordLevelTransform(localLevel, z, 1);	//���Ӷ���
				feature[coord] = 1;
			}
			else if(item.count == 1){
				Level localLevel = item.level;
				coord = coordLevelTransform(localLevel, z, 0);	//���ӵ���
				feature[coord] = 1;
			}
			else{
				//cout<<"error occur!"<<endl;
			}
		}

		Level prevLevel = -1;	int prev_count = -1;
		len_vec.assign(zero_len.begin(), zero_len.end());

		for(int j = localPack.size()-1; j >= 0; j--){
			Level localLevel = localPack[j].level;
			int localCount = localPack[j].count;

			if(prevLevel == -1 || (localLevel == prevLevel + 1 && prev_count == localCount)){
				len_vec[localCount]++;
				prevLevel = localLevel;
				prev_count = localCount;
			}
			else{
				checkSeq(len_vec, limit, localCardCombo, j, counts, countOfCount, z);
				len_vec[localCount] = 0;
				prevLevel = -1;
				prev_count = -1;
			}
		}
		if(prevLevel != -1){		//ĩβ���
			checkSeq(len_vec, limit, localCardCombo, localPack.size()-1, counts, countOfCount, z);
		}
		delete [] counts;
		delete [] countOfCount;
	}
	else{				//�����������������
		CardComboType localType = localCardCombo.comboType;
		auto &curPack = localCardCombo.packs;
		int coord;
		if(curPack.size() > 0){
			if(curPack[0].count == 4){
				if(curPack[curPack.size()-1].count == 4){		//ը��
					coord = coordLevelTransform(curPack[0].level, z, 6);
					feature[coord] = 1;
				}
				else if(curPack[curPack.size()-1].count == 2){	//�Ĵ�һ
					coord = coordLevelTransform(curPack[0].level, z, 7);
					feature[coord] = 1;
				}
				else{	//�Ĵ���
					coord = coordLevelTransform(curPack[0].level, z, 8);
					feature[coord] = 1;
				}
			}
			else if(curPack[0].count == 3){			//����������Ժϲ�����ʱ���ͷ����
				int coord;
				if(localType == CardComboType::TRIPLET){				//����
					coord = coordLevelTransform(curPack[0].level, z, 5);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::TRIPLET1){			//����һ
					coord = coordLevelTransform(curPack[0].level, z, 9);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::TRIPLET2){			//������
					coord = coordLevelTransform(curPack[0].level, z, 10);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::PLANE){				//�ɻ�
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 4);
						feature[coord] = 1;
						if(card.count != 3){
							break;
						}
					}
				}
				else if(localType == CardComboType::PLANE1){			//�ɻ�����
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 11);
						feature[coord] = 1;
						if(card.count != 3){
							break;
						}
					}
				}
				else if(localType == CardComboType::PLANE2){			//�ɻ���˫
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 12);
						feature[coord] = 1;
						if(card.count != 3){
							break;
						}
					}
				}
				else{
					//cout<<"ERROR!!!"<<endl;
				}
			}
			else if(curPack[0].count == 2){
				int coord;
				if(localType == CardComboType::PAIR){		//����
					coord = coordLevelTransform(curPack[0].level, z, 1);
					feature[coord] = 1;
				}
				else{										//˫˳
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 3);
						feature[coord] = 1;
					}
				}
			}
			else{
				int coord;
				if(localType == CardComboType::SINGLE){		//����
					coord = coordLevelTransform(curPack[0].level, z, 0);
					feature[coord] = 1;
				}
				else{										//��˳
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 2);
						feature[coord] = 1;
					}
				}
			}
		}
	}
}

void build_feature(struct Ddz * pDdz){
	//��һ������
	vector<int> cardOnHand;
	int hand_count = 0;
	for(int i = 0; pDdz->iOnHand[i] >= 0; i++){
		cardOnHand.push_back(pDdz->iOnHand[i]);
		int coord = coordTransform(pDdz->iOnHand[i], 0);
		feature[coord] = 1;
		hand_count += 1;
	}
	add_feature(cardOnHand, 0, 1);		//������������

	vector<int>().swap(cardOnHand);			//��������

	//�������λ��
	int selfPos = abs((int)(pDdz->cDir - pDdz->cLandlord));
	feature[dim1 * 0 + dim2 * 17 + 10] = selfPos;
	//����ʣ������
	feature[dim1 * 0 + dim2 * 17 + 14] = hand_count;

	//�ڶ�������
	for(int index = 0; index < 54; index++){
		if(remainCards[index]){
			int coord = coordTransform(index, 1);
			feature[coord] = 1;
		}
	}
	//�������λ��
	feature[dim1 * 1 + dim2 * 17 + 10] = selfPos;

	//����������
	vector<vector<int> > prev6;		//ǰ��(18��)����

	int num_play = pDdz->iOTmax;
	int count_in = 0;

	//��ʼ��ʣ����������
	vector<int>* handCardRemain = new vector<int>(3, 0);
	for(int i = 0; i < 3; i++){
		handCardRemain->at(i) = cardRemaining[i];
	}

	while(num_play > 0 && count_in < 18){		//6�ֹ�18��
		int cur_turn = count_in / 3;
		vector<int> cardsInThisTurn;
		for(int i = 0; pDdz->iOnTable[num_play][i] >= 0; i++){
			cardsInThisTurn.push_back(pDdz->iOnTable[num_play][i]);
			int coord = coordTransform(pDdz->iOnTable[num_play][i], count_in + 2);
			feature[coord] = 1;
		}
		add_feature(cardsInThisTurn, count_in + 2, 3);
		prev6.push_back(cardsInThisTurn);

		//�����ִΣ���ұ�ź�ʣ������
		feature[2 + dim1 * count_in + dim2 * 17] = cur_turn;
		int player = ((myPosition + 3) - (count_in % 3)) % 3;
		feature[2 + dim1 * count_in + dim2 * 17 + 10] = player;
		feature[2 + dim1 * count_in + dim2 * 17 + 14] = handCardRemain->at(player);
		handCardRemain->at(player) += cardsInThisTurn.size();
		num_play--;
		count_in++;
		vector<int>().swap(cardsInThisTurn);
	}

	//���Ĳ�����
	feature[dim1 * 20 + dim2 * 17 + 10] = selfPos;
	for(int x = 0; x < prev6.size(); x++){
		for(int y = 0; y < prev6[x].size(); y++){
			int coord = coordTransform(prev6[x][y], 20);
			feature[coord] = 1;
		}
	}
	vector<vector<int> >().swap(prev6);			//��������
	vector<int>().swap(cardOnHand);
}



/********LIAO**********/
//P030601-START���㼺�����Ʋ���
//����޶���:�ĺ��н�&÷��,����޶�ʱ��:15-02-12
void CalPla(struct Ddz * pDdz)
{
	int i;
	double dValueNow;
	double dValueMax=-9999;
	int iMax = 0;
	infoConvert( pDdz );

	HelpPla(pDdz);				//��Ҫ�����Ƽ�����pDdz->iPlaArr[],pDdz->iPlaCount
	dout << "[CalPla] candidates=" << pDdz->iPlaCount << " lastType=" << pDdz->iLastTypeCount << flush << endl;
	for(i=0;i<pDdz->iPlaCount;i++)
	{
		vector<Card> Actions;
		for (int j = 0;pDdz->iPlaArr[i][j] >= 0; j++)
			Actions.push_back(pDdz->iPlaArr[i][j]);
		CardCombo combo(Actions.begin(), Actions.end());
		HelpTakeOff(pDdz,i);	//����ȡ���˵�i���ƣ���ʣ����Ʒ���pDdz->iPlaOnHand[]
		//dout << i <<" : "<<print(combo)<<'\t';
		dValueNow = CalCardsValue(pDdz->iPlaOnHand);			//�������ƹ�ֵ
		if (dValueNow > dValueMax)
		{
			dValueMax = dValueNow;
			iMax = i;
		}
	}

		vector<Card> ActionCards;
		for (i = 0;pDdz->iPlaArr[iMax][i] >= 0; i++)
			ActionCards.push_back(pDdz->iPlaArr[iMax][i]);

		CardCombo myAction(ActionCards.begin(),ActionCards.end());
		//�����ƽ��д���
		vector<Card> myCards,remainCards;
		for(i=0;pDdz->iOnHand[i]!=-1;i++) myCards.push_back(pDdz->iOnHand[i]);
		CardCombo myCardCombo(myCards.begin(),myCards.end());
		ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
		comboDiv myCardsDiv = myComboDiv.div;
		sort(myCardsDiv.begin(),myCardsDiv.end());
		////dout << "Enter into Special Judge:"<<endl;

		iCanWin = false;
		comboDiv normal,king;//��������Ϣ����Ӯ�о֣�
		sort(myCards.begin(),myCards.end());
		removeCards(myCards, myAction.cards, remainCards);
		CardCombo remainCombo = CardCombo(remainCards.begin(), remainCards.end());
		ComboDiv remainDiv = remainCombo.div(remainCombo);
		comboDiv remaindiv = remainDiv.div;

		if(myCardCombo.packs.size() > 1 && myCardCombo.packs[0].count == 3 && myCardCombo.packs[1].count != 3){
			if(myCardCombo.packs.size() > 2 && (myCardCombo.packs)[1].level == 14 && (myCardCombo.packs)[2].level == 13){			//	�����ǲ�����ը
				pDdz->iToTable[0] = card_joker;
				pDdz->iToTable[1] = card_JOKER;
				pDdz->iToTable[2] = -1;
				return;

			}
		}
		comboDiv singles, pairs;
		for(vector<CardCombo>::iterator k = myCardsDiv.begin(); k!= myCardsDiv.end(); k++){//�Ƿ�����Ӯ
			CardCombo part = *k;
			if(part.getValue2()>=roundValWithDecay)
				king.push_back(part);
			else normal.push_back(part);
            if(part.comboType == CardComboType::SINGLE)
                singles.push_back(part);
            if(part.comboType == CardComboType::PAIR)
                pairs.push_back(part);
		}
			//��Ӯ�о��о�
	int num_normals = normal.size();
	int kicker_remain = singles.size() + pairs.size();

	int small_normals=singles.size() + pairs.size();
	for(vector<CardCombo>::iterator k = singles.begin(); k!= singles.end(); k++){//�Ƿ�����Ӯ
			CardCombo part = *k;
			if(part.comboLevel>10)//����K�ĵ��ƣ�  2
				small_normals--;
            else if(part.getValue2()>=7)
                small_normals--;
		}
    for(vector<CardCombo>::iterator k = pairs.begin(); k!= pairs.end(); k++){//�Ƿ�����Ӯ
			CardCombo part = *k;
			if(part.comboLevel>9)//����Q�Ķ���
				small_normals--;
            else if(part.getValue2()>=7)
                small_normals--;
		}
    for(vector<CardCombo>::iterator part = myCardsDiv.begin(); part!= myCardsDiv.end(); part++){
		if(part->comboType == CardComboType::TRIPLET){
			small_normals-=1;
		}
		if(part->comboType == CardComboType::PLANE){
			small_normals-=2;
		}

	}

	for(int j = 0; j < king.size(); j++){
		if(king[j].comboType == CardComboType::SINGLE || king[j].comboType == CardComboType::PAIR){
			kicker_remain -= 1;					//�ǿ�������ĵ��ƺͶ����м���
		}
	}
	int accumulate = 0;
	for(vector<CardCombo>::iterator part = myCardsDiv.begin(); part!= myCardsDiv.end(); part++){
		if(part->comboType == CardComboType::TRIPLET){
			if(kicker_remain > 0){
				accumulate += 1;				// �������Ե���һ�ţ����ۼ���������ټ���
			}
		}
		if(part->comboType == CardComboType::PLANE){
			if(kicker_remain > 1){
				num_normals -= 2;
				kicker_remain -= 2;
			}
		}
		if(part->comboType == CardComboType::BOMB){
			if(kicker_remain > 1){
				num_normals -= 2;
				kicker_remain -= 2;
			}
		}
	}
	num_normals = num_normals - min(kicker_remain, accumulate);		// ��������㹻�ͼ�������������������ֻ�ܰѼ������д��Ƶ�����

	//dout<<"I am "<< pDdz->cDir<<" num_normals: "<<num_normals<<" accumulate: "<<accumulate<<"  "<<print(king)<<"  "<<print(normal)<<endl;


		//��һ���ƾ�ֱ�ӳ�����
		if(remainCombo.comboType == CardComboType::PASS){
			 iCanWin = true;
		}
		//���ֻ��һ�ŷǿ��֣�һֱ����������Ӯ
		if(num_normals<=1 && king.size()>0){
			iCanWin = true;
		}

/***************�ȴ�����һ���жϣ�ʹ���������******************/
	/* DEBUG: log key decision info */
	dout << "CalPla: pos=" << myPosition << " lastType=" << pDdz->iLastTypeCount
		<< " plaCount=" << pDdz->iPlaCount << " lastPlayer=" << lastPlayer
		<< " iMax=" << iMax << " remainL=" << cardRemaining[Landlord]
		<< " canWin=" << iCanWin << endl;
	if(pDdz->iPlaCount > 0) {
		dout << "  Candidates:";
		for(int dbg=0; dbg<min(5,pDdz->iPlaCount); dbg++) {
			int arr[21]; int ai=0;
			for(; pDdz->iPlaArr[dbg][ai]>=0; ai++) arr[ai]=pDdz->iPlaArr[dbg][ai];
			arr[ai]=-1;
			dout << " [" << AnalyzeTypeCount(arr) << ":" << AnalyzeMainPoint(arr) << "]";
		}
		dout << endl;
	}

	if(pDdz->iLastTypeCount!=0 and pDdz->iPlaCount>0){//�������Ƶ�����
		if(iCanWin && remainCombo.comboType != CardComboType::PASS){			//�����Ӯ�ˣ�����ֱ�ӳ����ֶ�ȡʤ����ʵ
			sort(king.begin(),king.end());
			for(int k = king.size()-1; k >= 0; k--){
				if(lastValidCombo.canBeBeatenBy(king[k]))		//����С�Ŀ���
				{
					int index = 0;
					for(int j = 0; j < king[k].cards.size(); j++){
						pDdz->iToTable[index] = king[k].cards[j];
						index++;
					}
					pDdz->iToTable[index] = -1;
					return;
				}
			}
		}				// ������ֶ���������ֻ�ܳ�normal����
		if(!iCanWin){
/***************Liao��*****************/
            bool bigSingle=false;//�����ж��ϼҶ��Ѵ��ƵĴ�С�����Ƿ�����
            bool bigPair=false;//�����ж��ϼҶ��Ѵ��ƵĴ�С�����Ƿ�����
            bool mybigSingle=false;//�����ж��Լ����ƵĴ�С�����Ƿ�����
            bool mybigPair=false;//�����ж��Լ����ƵĴ�С�����Ƿ�����
			//����û��Ҫ�û���ܣ���һ���������ô�
			if(myAction.comboType == CardComboType::ROCKET and lastValidCombo.comboType==CardComboType::SINGLE){
				vector<Card> the_joker;
				the_joker.push_back(card_joker);
				myAction = CardCombo(the_joker.begin(),the_joker.end());
				pDdz->iToTable[0] = card_joker;
				pDdz->iToTable[1] = -1;
				return;
			}

            //if(myAction.comboType==CardComboType::BOMB){dout<<"small_normals:"<<small_normals<<endl;}
			if(lastValidCombo.comboType==CardComboType::SINGLE)
                    {
                        if(lastValidCombo.comboLevel>10)bigSingle=true;
                    }
                    if(lastValidCombo.comboType==CardComboType::PAIR)
                    {
                        if(lastValidCombo.comboLevel>9)bigPair=true;
                    }
            if(myAction.comboType==CardComboType::SINGLE)
                    {
                        if(myAction.comboLevel>10)mybigSingle=true;
                    }
                    if(myAction.comboType==CardComboType::PAIR)
                    {
                        if(myAction.comboLevel>9)mybigPair=true;
                    }

			if(myPosition == FarmerB)//�����ũ���ң�Ҫ��������
			{
			    //if(myAction.comboType==CardComboType::BOMB){dout<<"small_normals:"<<small_normals<<endl;}
				if(lastPlayer == FarmerA){
					if(cardRemaining[Landlord] == 1){
                        if(lastValidCombo.getValue2()>=7){
                            pDdz->iToTable[0] = -1;//����
                            return;
                        }
                        else
                        {
                            CardCombo bigS(--myCards.end(),myCards.end());
                            if(lastValidCombo.comboType==CardComboType::SINGLE and
                                lastValidCombo.canBeBeatenBy(bigS))
                            {
                                pDdz->iToTable[0] = *(myCards.rbegin());
                                pDdz->iToTable[1] = -1;
                                return;
                            }
                        }
					}

					/* Smart coop: pass to teammate only when they have <=2 cards AND strong play. */
					if(cardRemaining[FarmerA] <= 2 && lastValidCombo.getValue2() >= 7){
						pDdz->iToTable[0] = -1; return;
					}
				}
				if(lastPlayer == Landlord){
					if(cardRemaining[Landlord] == 1){
						CardCombo bigS(--myCards.end(),myCards.end());
						if(lastValidCombo.comboType==CardComboType::SINGLE and
							lastValidCombo.canBeBeatenBy(bigS))
						{
							pDdz->iToTable[0] = *(myCards.rbegin());
							pDdz->iToTable[1] = -1;
							return;
						}
					}
					/***********���ĵ���ʱ������***********/
					//�Լ����ƺܴ󣬵���ʣ����������ĳֵ���Լ�ɢ��С���ƺܶ࣬��Ϊũ���ҵ�����
					//����Լ�ɢ��С����������



				}
			}
			if (myPosition == FarmerA)//�����ũ��ף�ֻҪ������ڶ����У�KΪ���ޣ�
			{
				if(lastPlayer == FarmerB){
					/* Smart coop: pass to teammate only when they have <=2 cards AND strong play. */
					if(cardRemaining[FarmerB] <= 2 && lastValidCombo.getValue2() >= 7){
						pDdz->iToTable[0] = -1; return;
					}
				}
				if(lastPlayer == Landlord){
					if(cardRemaining[Landlord] == 1){
						CardCombo bigS(--myCards.end(),myCards.end());
						if(lastValidCombo.comboType==CardComboType::SINGLE and
							lastValidCombo.canBeBeatenBy(bigS))
						{
							pDdz->iToTable[0] = *(myCards.rbegin());
							pDdz->iToTable[1] = -1;
							return;
						}
					}
					/***********���ĵ���ʱ������***********/

				}
			}
			if(myPosition == Landlord){
				/* Landlord must stay aggressive (DouZero+ principle).
				 * If responding to a farmer and we have JOKER/2/bomb, never pass.
				 * Losing control as landlord usually means losing the game. */
				/* Landlord aggression with card conservation.
				 * Play JOKER/2 only when the play we're beating is worth it
				 * (level>=10 i.e. K/A/2) or opponent has <=3 cards left.
				 * Don't waste JOKER on beating a 3 or 4. */
				if(pDdz->iPlaCount > 0 && iMax >= 0 && iMax < pDdz->iPlaCount) {
					int actionArr[21]; int ai=0;
					for(; pDdz->iPlaArr[iMax][ai]>=0; ai++)
						actionArr[ai]=pDdz->iPlaArr[iMax][ai];
					actionArr[ai]=-1;
					int mainPt = AnalyzeMainPoint(actionArr);
					bool isBig = (mainPt >= 12);  /* JOKER=14, joker=13, 2=12 */
					bool isBomb = IsType2Bomb(actionArr) || IsType1Rocket(actionArr);
					int respLevel = pDdz->iLastMainPoint;
					bool oppLow = (cardRemaining[FarmerA] <= 3 || cardRemaining[FarmerB] <= 3);
					/* Play big cards only when: opponent is low OR beating a big card */
					if((isBig && (respLevel >= 10 || oppLow)) || (isBomb && oppLow)) {
						for (i = 0; i < ai; i++)
							pDdz->iToTable[i] = pDdz->iPlaArr[iMax][i];
						pDdz->iToTable[ai] = -1;
						return;
					}
					/* Don't force big cards on small plays. Let normal
					 * valuation decide. JOKER on a 3 is wasteful. */
				}
				if(cardRemaining[FarmerA]==1 || cardRemaining[FarmerB]==1){
					CardCombo bigS(--myCards.end(),myCards.end());
					if(lastValidCombo.comboType==CardComboType::SINGLE and
						lastValidCombo.canBeBeatenBy(bigS))
						{
							pDdz->iToTable[0] = *(myCards.rbegin());
							pDdz->iToTable[1] = -1;
							return;
						}
				}

				/*if((myAction.comboType==CardComboType::BOMB or myAction.comboType==CardComboType::ROCKET) and (small_normals>3 or king.size()<=small_normals-2 ) ) {
                    pDdz->iToTable[0] = -1;//����
                        return;
				}*/

				/***********����ũ��ʱ������***********/
                if(myAction.comboType==CardComboType::BOMB)dout<<"small_normals:"<<small_normals<<endl;

			}
		}
	}

	/* ISMCTS (Whitehouse et al., CIG 2011): use tree search when <=8 candidates */
	if(pDdz->iPlaCount > 0 && pDdz->iPlaCount <= 8) {
		vector<int> ismctsAction = ismcts_search(pDdz, 300, 1.0f);
		if(!ismctsAction.empty()) {
			for(int is_i = 0; is_i < pDdz->iPlaCount; is_i++) {
				bool match = true; int is_j = 0;
				for(; pDdz->iPlaArr[is_i][is_j] >= 0; is_j++)
					if(is_j >= (int)ismctsAction.size() || pDdz->iPlaArr[is_i][is_j] != ismctsAction[is_j])
						{ match = false; break; }
				if(match && is_j == (int)ismctsAction.size()) { iMax = is_i; break; }
			}
		}
	}

	dout << "Final: iMax=" << iMax << " plaCount=" << pDdz->iPlaCount		<< " iToTable[0]=" << (iMax>=0&&pDdz->iPlaArr[iMax][0]>=0?pDdz->iPlaArr[iMax][0]:-1) << endl;
	/* EMERGENCY: never pass when landlord is about to win (<=2 cards).
	 * This is the fix for the Game 8 bug where B had pair 5s to beat
	 * pair 4s but passed, letting C win immediately. */
	if(cardRemaining[Landlord] <= 2 && pDdz->iPlaCount > 0) {
		/* Force play the smallest legal response */
		if(iMax < 0 || iMax >= pDdz->iPlaCount || pDdz->iPlaArr[iMax][0] < 0)
			iMax = 0;
	}
	/* Also: when responding to landlord with any legal play, never pass.
	 * If iMax is invalid for some reason, force first candidate. */
	if(pDdz->iLastTypeCount != 0 && pDdz->iPlaCount > 0
		&& iMax >= pDdz->iPlaCount) {
		iMax = 0;
	}

	if(iMax>-1){
		for (i = 0;pDdz->iPlaArr[iMax][i] >= 0; i++)
        {
			pDdz->iToTable[i] = pDdz->iPlaArr[iMax][i];
        }
	}
	else{
		i = 0;
	}
	pDdz->iToTable[i] = -1;

}

//P030601-END
string print(vector<CardCombo> divCombos){
	string s; short x;
	for(int i=0;i<divCombos.size();++i){
		CardCombo combo = divCombos[i];
		for(Card c : combo.cards){
			if(card2level(c) == 7){
				s+='X';
				continue;
			}
			else if(card2level(c) == 8){
				s+='J';
				continue;
			}
			else if(card2level(c) == 9){
				s+='Q';
				continue;
			}
			else if(card2level(c) == 10){
				s+='K';
				continue;
			}
			else if(card2level(c) == 11){
				s+='A';
				continue;
			}
			else if(card2level(c) == 12){
				s+='2';
				continue;
			}
			else if(card2level(c) == 13){
				s+="joker";
				continue;
			}
			else if(card2level(c) == 14){
				s+="JOKER";
				continue;
			}
			x = card2level(c) + '3';
			s += static_cast<char>(x);
		}
		s+=' ';
	}
	return s;
}
string print(CardCombo combo){
	string s; short x;
	for(Card c : combo.cards){
		if(card2level(c) == 7){
			s+='X';
			continue;
		}
		else if(card2level(c) == 8){
			s+='J';
			continue;
		}
		else if(card2level(c) == 9){
			s+='Q';
			continue;
		}
		else if(card2level(c) == 10){
			s+='K';
			continue;
		}
		else if(card2level(c) == 11){
			s+='A';
			continue;
		}
		else if(card2level(c) == 12){
			s+='2';
			continue;
		}
		else if(card2level(c) == 13){
			s+="joker";
			continue;
		}
		else if(card2level(c) == 14){
			s+="JOKER";
			continue;
		}
		x = card2level(c) + '3';
		s += static_cast<char>(x);
	}
	s+=' ';
	return s;
}


/*
	������ģ��Ծֲ���
*/

//ģ��һ�γ���
void Round(struct Ddz * pDdz, string& pCmd, vector<string>& answer){
		InputMsg(pDdz, pCmd, true);			  //������Ϣ
		AnalyzeMsg(pDdz);		          //����������Ϣ
		OutputMsg(pDdz, answer, true);		  //�����Ϣ
		//CalOthers(pDdz);		          //������������
}

//���ƽ���ж�
void ProcessBid(vector<string> answer, string& pCmd, vector<string>& deliveredCards, pair<char, char>& maxBider){
	string s = *(answer.end() - 3);
	if(s[5] - maxBider.second > 0){
		maxBider.first = s[4];
		maxBider.second = s[5];
	}
    if(s[5] == '3'|| s[4] == 'C'){
    	char ch = s[4];
		if(maxBider.second == '0'){
			pCmd = "GAMEOVER B";
			invalidRound += 1;
			return;
		}
    	string tmp = *(deliveredCards.rbegin());
		string Command = "LEFTOVER ";
    	pCmd = Command + maxBider.first + tmp;
		currentPlayer = maxBider.first - 'A';		//���µ�ǰ�����ߣ��е����������ȳ���
	}
	else{
		currentPlayer = (currentPlayer + 1) % 3;	//�ϼҽ����ֵ��¼ҽ�
		pCmd = "BID WHAT";
	}
}

//�ж���Ϸ�Ƿ����
void ProcessEndGame(struct Ddz * pDdz1, struct Ddz * pDdz2, struct Ddz * pDdz3, string& pCmd){
	string tmp;
	if(pDdz1->iOnHand[0] == -1){
		tmp = pDdz1->cDir;
		pCmd = "GAMEOVER " + tmp;
	}
	else if(pDdz2->iOnHand[0] == -1){
		tmp = pDdz2->cDir;
		pCmd = "GAMEOVER " + tmp;
	}
	else if(pDdz3->iOnHand[0] == -1){
		tmp = pDdz3->cDir;
		pCmd = "GAMEOVER " + tmp;
	}
	else{
		pCmd = "PLAY WHAT";
		currentPlayer = (currentPlayer + 1) % 3;
	}
}

//����
void buildDeliveredCards(vector<string>& deliveredCards){
	vector<string> nVector = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20",
	                       "21","22","23","24","25","26","27","28","29","30","31","32","33","34","35","36","37","38","39","40",
						   "41","42","43","44","45","46","47","48","49","50","51","52","53"};
	random_shuffle(nVector.begin(), nVector.end());

	// for(int i =0 ; i < nVector.size(); i++){
	// 	cout<<nVector[i];
	// }
	// cout<<endl;
	vector<string>().swap(deliveredCards);
	string tmp = "DEAL A";
	for(int i = 0; i < 17; i++){
		tmp = tmp + nVector[i] + ",";
	}
	tmp.pop_back();
	deliveredCards.push_back(tmp);
	tmp = "DEAL B";
	for(int i = 17; i < 34; i++){
		tmp = tmp + nVector[i] + ",";
	}
	tmp.pop_back();
	deliveredCards.push_back(tmp);
	tmp = "DEAL C";
	for(int i = 34; i < 51; i++){
		tmp = tmp + nVector[i] + ",";
	}
	tmp.pop_back();
	deliveredCards.push_back(tmp);
	tmp = "";
	for(int i = 51; i < 54; i++){
		tmp = tmp + nVector[i] + ",";
	}
	tmp.pop_back();
	deliveredCards.push_back(tmp);
}

//����
void DeliverCards(string& pCmd, vector<string>& deliveredCards, vector<string>& answer){
	int i = (answer.size() - 3) % 3;
	pCmd = deliveredCards[i];
}

//������һظ���Ϣ����ƽ̨��ָʾ����
void platformAction(string& pCmd, vector<string>& answer, vector<string>& deliveredCards,
					struct Ddz * pDdz1, struct Ddz * pDdz2, struct Ddz * pDdz3, pair<char,char>& maxBider)
{
	int n = answer.size();
	int m = pCmd.length();
	if(n != 0){
		if(n % 3 == 0){
			string s = *answer.rbegin();
			switch(s[0]){
				case 'N':	// name
						pCmd = "INFO 1,1,1,1,1,2100,15";
						currentPlayer = 0;		//��ʼ�׶β���������
						break;
				case 'O':	//OK
						switch(s[3]){
							case 'I':		// ok info
									DeliverCards(pCmd, deliveredCards, answer);
									currentPlayer = 0;		//��ʼ�׶β���������
									break;
							case 'D':
								    pCmd = "BID WHAT";
									currentPlayer = 0;		//��ʼ�׶β���������
							        break;
							case 'B':		// ok bid
									ProcessBid(answer, pCmd, deliveredCards, maxBider);
									break;
							case 'L':		// ok leftover
									pCmd = "PLAY WHAT";
									break;
							case 'P':			// ok play
									ProcessEndGame(pDdz1, pDdz2, pDdz3, pCmd);
									break;
							case 'G':		// ok gameover

									break;
							default:
									;
						}
						break;
				case 'G':	//gameover
						break;
				default:
					;
			}
		}
		else{
			string s = *answer.rbegin();
			if(s[0] == 'B' || s[0] == 'P'){
				pCmd = s;
			}
			if(s[0] == 'O'){
				if((s[3] == 'B' || s[3] == 'P')){
				pCmd = *(answer.end() - 2);
				}
				else if(s[3] == 'D'){
					DeliverCards(pCmd, deliveredCards, answer);
				}
			}
		}
	}
}

//���ֽ׶Σ���ʼ��ȫ�ֱ���
void InitParams(){
	othersCards.assign(zeros.begin(), zeros.end());
	memset(cardRemaining, 0, 3 * sizeof(int));
	DirConvertPara = 0;
	myPosition = 0;
	lastPlayer = 0;
	lastTurn = 0;
	iCanWin = false;
}

//ģ��һ�ֱ���
int Play(){
	string pCmd;				//ģ��ƽ̨��������
	vector<string>().swap(answer);		//��һ�Ӧ��Ϣ����
	currentPlayer = 0;			//��ʼ��������ΪA
	int Winner = -1;

	pair<char,char> maxBider = make_pair('A', '0');

	// struct Ddz tDdz1, *pDdz1=&tDdz1;
	// struct Ddz tDdz2, *pDdz2=&tDdz2;
	// struct Ddz tDdz3, *pDdz3=&tDdz3;
	struct Ddz *pDdz1 = new Ddz;
	struct Ddz *pDdz2 = new Ddz;
	struct Ddz *pDdz3 = new Ddz;
	InitTurn(pDdz1);			//��ʼ������
	InitTurn(pDdz2);			//��ʼ������
	InitTurn(pDdz3);			//��ʼ������
	InitParams();				//��ʼ��ȫ�ֱ���

	buildDeliveredCards(deliveredCards);

	pCmd = "DOUDIZHUVER 1.0";
	vector<struct Ddz *> pDdzVector;
	pDdzVector.push_back(pDdz1);
	pDdzVector.push_back(pDdz2);
	pDdzVector.push_back(pDdz3);

	int exitPlayer = -1;		//ʤ����־
	int roll = 0;				//���ִ���ͳ�Ʊ���
	while(1){
		for(int i = 0; i < 3; i++){				//ģ��һ�γ���
			int index = (currentPlayer + i) % 3;
			//���1����ѵ��Ȩ�أ����2��3����ԭʼȨ��
			baseCardsWeight = 1.0;
			roundValWithDecay = 7.0;
			SeqDecayRate = 1.0;
			RocketVal = 20;


			if(pDdzVector[index]->iStatus == 0){
				Winner = index;
				exitPlayer = 1;
				break;
			}
			Round(pDdzVector[index], pCmd, answer);

			string lastCommand = *(answer.rbegin());
			int n = lastCommand.length();
			if(lastCommand[n-2] == '-'){						//���pass
		        string prev = *(answer.end()-2);
				if(prev[3] == 'L'){			// ��ͷpass
					return 1;
				}
				string tmp1 = *(answer.end() - 4);	//����pass
				string tmp2 = *(answer.end() - 7);
				if(tmp1[tmp1.length()-2] == '-' && tmp2[tmp2.length()-2] == '-'){
					return 1;	//Ĭ��Bʤ����ֻҪ����A����
				}
			}

			assert(pDdzVector[0]->iOnHand[0] < 60);
			assert(pDdzVector[1]->iOnHand[0] < 60);
			assert(pDdzVector[2]->iOnHand[0] < 60);

			platformAction(pCmd, answer, deliveredCards,pDdzVector[0], pDdzVector[1], pDdzVector[2], maxBider);		//����ƽ̨����
		}
		if(exitPlayer == 1)				//����ʤ�����˳�ѭ��
			break;
		roll++;
	}
	delete pDdz1;
	delete pDdz2;
	delete pDdz3;
	return Winner;
}

/*
	������ģ��Ծֲ���
*/



int main()
{
	clock_t startTime, endTime;
	int totalRounds = 1200;
	int winTimes = 0;
	int winner;
	bool selfPlay = false;		// �Ƿ����ҶԾ�:��Ϊfalse���ԭ���ĳ���һ��
	if(selfPlay){
		startTime = clock();

		// ��txt��ȡ����
		ifstream params("params.txt");
		float param = 0;
        while(params >> param){
			param_vec.push_back(param);
		}

		for(int step = 0; step < totalRounds; step++){
			if(step % 10 == 0){
				cout<<"WinRate: "<<step<<" : "<<float(winTimes) / float(step - invalidRound)<<"    validRound: "<<step - invalidRound<<endl;
			}
			if(step % 100 == 0){
				cout<<"WinRate: "<<step<<" : "<<float(winTimes) / float(step - invalidRound)<<"    validRound: "<<step - invalidRound<<endl;
				srand((unsigned)time(NULL));	//ˢ�����������
			}
			winner = Play();
			if(winner == 0){
				winTimes += 1;
			}
		}
		endTime = clock();
		cout<<"Time: "<<endTime - startTime<<" ms."<<endl;
		float winRate = winTimes / float(totalRounds - invalidRound);
		ofstream writeParams("WinRate.txt", ios::trunc);
		writeParams<<winRate;
		printf("Win rate: %f\n", winRate);

		writeParams.close();
		params.close();
		dout.close();
		system("pause");
	}
	else{
		struct Ddz tDdz, *pDdz=&tDdz;
		InitTurn(pDdz);			//��ʼ������
		while(pDdz->iStatus!=0)
		{
			InputMsg(pDdz, " ", false);			//������Ϣ
			AnalyzeMsg(pDdz);		//����������Ϣ
			vector<string> tmp;
			OutputMsg(pDdz, tmp, false);		//�����Ϣ
			CalOthers(pDdz);		//������������
		}
		dout.close();
	}
	return 0;
}
