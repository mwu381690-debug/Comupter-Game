#define kPlayerName "TeamA"
#define kPlaMax 500
#include "DdzV200.h"
#include <assert.h>
#include "opponent_model.h"

//不知道怎么输队
#define DLLEXPORT extern "C" __declspec(dllexport)

vector<int> othersCards(15);//记牌器
vector<int> remainCards(54);//记牌器2号

int AtoB_dedication=0;
int cardRemaining[3];//余牌数量统计
int DirConvertPara;//位置矫正参数
int myPosition; //数字方位 0,1,2 Landlord,FarmerA,FarmerB
int lastPlayer; //上一位合法玩家的数字方位
int lastTurn;	//上一个有效牌型的轮数
bool iCanWin;
int currentPlayer; //指示每轮从哪一方开始
vector<float> TrainParams;		//待训练的估值函数参数


//优化用参数
float baseCardsWeight = 1.0;            //衡量牌的权重
float roundValWithDecay = 7;          //衡量手数的权重
float SeqDecayRate =  1;			    //衡量顺子的权重
float RocketVal = 20;
vector<float> param_vec;				//参数向量

int sum = 0;			//对弈获得的总分数

int invalidRound = 0;			//无效的局数

//神经网络参数
int channels = 21, height = 19, width = 15;
int batch_size = 32;
int num_classes = 309;
int dim1 = height * width; int &dim2 = width;

//初始化
//vector<float> feature(channels * height * width, 0);
int * feature;
//vector<float> zero_vec(channels * height * width, 0);
vector<int> limit = {-1, 5, 3, 2};
vector<int> len_vec(4, 0);
vector<int> zero_len(4, 0);
vector<int> zeros(15, 0);


//模拟用变量
vector<string> deliveredCards(4, "default");	//储存分牌结果
vector<string> answer;		//玩家回应信息集合

// 牌的组合，用于计算牌型
typedef struct CardCombo
{
	// 表示同等级的牌有多少张
	// 会按个数从大到小、等级从大到小排序
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
	vector<Card> cards; // 原始的牌，未排序
	vector<CardPack> packs; // 按数目和大小排序的牌种
	CardComboType comboType; // 算出的牌型
	Level comboLevel = 0; // 算出的大小序
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
	float getValue() const;//获取value
	float getValue2() const;//获取value
	float getSeqValue(comboDiv& ComboSeq);//获取序列总value
	/*---------------减少重复操作的函数---------------*/
	float MergeValue(CardCombo main, comboDiv DivRemain);
	CardCombo MergeCombo(comboDiv& singles, comboDiv& pairs);	//合并函数
	void ValueCompare(vector<Card>& divcards,vector<Card>& cuts, comboDiv& DivRemain, float& MaxValue);
	bool RemainValueCompare(vector<Card>& remainCards, ComboDiv& FinalSeq, CardCombo& myAction);
	float getExValue(comboDiv& Seq, short mainlv, short dlv, short k);
	/*查个数最多的CardPack递减了几个,用于计算顺子什么的 */
	int findMaxSeq() const;
	Seq findSeq1() const; //检测单顺
	Seq findSeq2() const; //检测双顺
	Seq findSeq3() const; //检测飞机
	Seq findSeq4() const; //检测航天飞机
	CardCombo() : comboType(CardComboType::PASS) {} // 创建一个空牌组
	//迭代法拆牌
	ComboDiv div(const CardCombo& remains){
		if(remains.comboType == CardComboType::PASS){//如果分完了，那就直接返回空序列了
			ComboDiv FinalDiv;
			FinalDiv.Value = 0;
			return FinalDiv;
		}
		else{
			// 每种牌有多少个
			short counts[MAX_LEVEL + 1] = {};

			// 同种牌的张数（有多少个单张、对子、三条、四条）
			short countOfCount[5] = {};

			Level check[MAX_LEVEL+1];
			for (Card c : remains.cards)//用桶排还原牌数
				counts[card2level(c)]++;
			for (Level l = 0; l <= MAX_LEVEL; l++)
				if (counts[l])
					countOfCount[counts[l]]++;
			// 用最多的那种牌总是可以比较大小的
			comboLevel = remains.packs[0].level;

			// 计算牌型
			// 按照 同种牌的张数 有几种 进行分类
			vector<int> kindOfCountOfCount;
			for (int i = 0; i <= 4; i++)
				if (countOfCount[i])
					kindOfCountOfCount.push_back(i);
			sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

			int curr;
			vector<Card> divcards = remains.cards;//向下追溯的牌
			vector<Card> cuts;		//截取的牌
			CardCombo CutCombo;		//截取的牌组成的Combo
			ComboDiv FinalDiv;		//最终的返回值
			comboDiv DivRemain;  	//最终Combo序列
			comboDiv TempSeq;		//暂存拆牌牌型的临时序列
			float MaxValue = -1000;		//暂存Value，最大Value
			Seq seq1 = remains.findSeq1();
			Seq seq2 = remains.findSeq2();
			Seq seq3 = remains.findSeq3();
			Seq seq4 = remains.findSeq4();
			/*
				目的：截取一部分牌，剩下的继续分，直到余牌是牌型
				返回值：已经有序的牌组
			*/
			switch (kindOfCountOfCount.size())
			{
			case 1: // 只有一类牌
				curr = countOfCount[kindOfCountOfCount[0]];
				switch (kindOfCountOfCount[0])
				{
				case 1:
					// 只有若干单张,急需找王炸和顺子
					if(curr >= 2 && remains.packs[1].level == level_joker){
						//初始化三连~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));

						for (auto c = divcards.begin(); c != divcards.end();) //寻找大小王，cut一组
						{
							if(card2level(*c) == level_joker or card2level(*c) == level_JOKER){//找到大小王就转移
								cuts.push_back(*c);
								c = divcards.erase(c); //边切边删
							}
							else c++;
						}
						//牌型递归部
						ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					if(seq1.len >= 5  && seq1.level<= MAX_STRAIGHT_LEVEL){//单顺
						//初始化三连~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
							curr = seq1.len;
							for (auto c = divcards.begin(); c != divcards.end();) //寻找单顺，cut一组
							{
								Level tmplv = card2level(*c);
								if(tmplv > seq1.level - curr && tmplv<=seq1.level && check[tmplv]<1 ){//找到顺子序列就转移【单】
									cuts.push_back(*c);
									c = divcards.erase(c); //边切边删
									check[tmplv]++;
									if(cuts.size()>=5){
										ValueCompare( divcards, cuts, DivRemain, MaxValue);
									}
								}
								else c++;
							}
					//	ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					{//就剩单张了
						//初始化三连~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));

						cuts.push_back(*(divcards.begin())); //单张cut第一个
						divcards.erase(divcards.begin());
						//牌型递归部
						ValueCompare( divcards, cuts, DivRemain, MaxValue);
					}
					break;
				case 2:
					// 只有若干对子
					if(seq2.len >= 3  && seq2.level<= MAX_STRAIGHT_LEVEL){//双顺
						//初始化三连~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
							curr = seq2.len;
							for (auto c = divcards.begin(); c != divcards.end();) //寻找双顺，cut一组
							{
								Level tmplv = card2level(*c);
								if(tmplv > seq2.level - curr && tmplv<=seq2.level && check[tmplv]<2 ){//找到顺子序列就转移【双】
									cuts.push_back(*c);
									c = divcards.erase(c); //边切边删
									check[tmplv]++;
									if(cuts.size()>=6 && cuts.size() % 2 == 0){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
								}
								else c++;
							}

					}
					{//都是对儿
						//初始化三连~~
						divcards = remains.cards;
						cuts.clear();
						memset(check,0,sizeof(check));
						for (auto c = divcards.begin(); c != divcards.end();) //寻找对子，cut一组
						{
							if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
								cuts.push_back(*c);
								c = divcards.erase(c); //边切边删
								if(cuts.size()>=2){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
							}
							else c++;
						}

					}
					break;
				case 3:// 只有若干三条
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //寻找三条，cut一组
					{

						if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
							if(cuts.size()>=3){
                                        ValueCompare( divcards, cuts, DivRemain, MaxValue);
                                    }
						}
						else c++;

					}
					break;
				case 4:// 只有若干炸弹
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //寻找炸弹，cut一组
					{
						if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
						}
						else c++;
					}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);
					break;
				}
				break;
			case 4: // 有多类牌
			case 3:
			case 2:
				curr = countOfCount[kindOfCountOfCount[1]];
				if(seq4.len >= 2  && seq4.level<= MAX_STRAIGHT_LEVEL){//航天飞机！！！
					//初始化三连~~
					/*
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
						curr = seq4.len;
						for (auto c = divcards.begin(); c != divcards.end();) //寻找航天飞机主牌，cut一组
						{
							Level tmplv = card2level(*c);
							if(tmplv > seq4.level - curr && tmplv<=seq4.level && check[tmplv]<4 ){//找到顺子序列就转移【四】
								cuts.push_back(*c);
								c = divcards.erase(c); //边切边删
								check[tmplv]++;
							}
							else c++;
						}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);*/
				}
				if(countOfCount[4]){
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //寻找炸弹，cut一组
					{
						if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
						}
						else c++;
					}
					ValueCompare( divcards, cuts, DivRemain, MaxValue);
				}
				if(seq3.len >= 2  && seq3.level<= MAX_STRAIGHT_LEVEL){//飞机！！！
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq3.len;
					for (auto c = divcards.begin(); c != divcards.end();) //寻找飞机主牌，cut一组
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq3.level - curr && tmplv<=seq3.level && check[tmplv]<3 ){//找到顺子序列就转移【三】
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
							check[tmplv]++;
							if(cuts.size()>=6 && cuts.size() % 3 == 0){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}
				}

				if(countOfCount[3])// 只有若干三条
				{
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //寻找三条，cut一组
					{
						if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
							if(cuts.size()>=3){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}

				}

				if(seq2.len >= 3  && seq2.level<= MAX_STRAIGHT_LEVEL){//双顺
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq2.len;
					for (auto c = divcards.begin(); c != divcards.end();) //寻找双顺，cut一组
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq2.level - curr && tmplv<=seq2.level && check[tmplv]<2 ){//找到顺子序列就转移【双】
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
							check[tmplv]++;
							if(cuts.size()>=6 && cuts.size() % 2 == 0){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;

					}
				}

				if(seq1.len >= 5  && seq1.level<= MAX_STRAIGHT_LEVEL){//单顺
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					curr = seq1.len;
					for (auto c = divcards.begin(); c != divcards.end();) //寻找单顺，cut一组
					{
						Level tmplv = card2level(*c);
						if(tmplv > seq1.level - curr && tmplv<=seq1.level && check[tmplv]<1 ){//找到顺子序列就转移【单】
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
							check[tmplv]++;
							if(cuts.size()>=5){
								ValueCompare( divcards, cuts, DivRemain, MaxValue);
							}
						}
						else c++;
					}
				}
				if(countOfCount[2])// 只有若干对儿
				{
					//初始化三连~~
					divcards = remains.cards;
					cuts.clear();
					memset(check,0,sizeof(check));
					for (auto c = divcards.begin(); c != divcards.end();) //寻找二条，cut一组
					{
						if(card2level(*c) == remains.packs[0].level){//那就找第一组吧
							cuts.push_back(*c);
							c = divcards.erase(c); //边切边删
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
	/*通过Card（即short）类型的迭代器创建一个牌型,计算出牌型和大小序等
	 * 假设输入没有重复数字（即重复的Card）*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end);
	/*判断指定牌组能否大过当前牌组（这个函数不考虑过牌的情况！）*/
	bool canBeBeatenBy(const CardCombo& b) const;
}CARDCOMBO;
CardCombo HelpPass(struct Ddz * pDdz);	//主动出牌
void setOthersCards( Ddz * pDdz );		//余牌统计
//手牌移除函数，vector版HelpTakeOff
void removeCards(vector<Card>& ownCards, vector<Card>& deleteCards, vector<Card>& remainCards);
void infoConvert( Ddz * pDdz );	//信息转换函数，用于衔接Botzone函数和Ddz平台变量
CardCombo lastValidCombo; //上一位合法玩家的牌型

//余牌统计
void setOthersCards( Ddz * pDdz ){

	//更新记牌器1
	for(int i=0;i<othersCards.size();++i){
		othersCards[i] = 0;
	}
	bool allcards[54];
	for(int i = 0;i<54;++i){
		allcards[i]=true;//将所有牌标记为存在
	}
	//将所有的历史出牌的情况标记为已经出过。
	cardRemaining[Landlord] = 20;//Landlord
	cardRemaining[FarmerA] = 17;//FarmerA
	cardRemaining[FarmerB] = 17;//FarmerB
	for(int turn=0; pDdz->iOnTable[turn][0]!=-2; turn++){
		for(int i=0;pDdz->iOnTable[turn][i]!=-1;i++){
			allcards[ pDdz->iOnTable[turn][i] ] = false;//曾经出过，将其标记为已经被出过。
			cardRemaining[turn%3] -= 1;//对应身份的人的手牌相应减少
		}
	}


	//将自己手里还有的牌标记为已知的牌。
	//将地主牌标记为已知的牌。
	for(int i=0;i<3;i++) allcards[pDdz->iLef[i]] = false;
	for(int i=0;pDdz->iOnHand[i]!=-1;i++) allcards[pDdz->iOnHand[i]] = false;
	//现在剩下的为true的牌就是没有的牌了。
	for(Card i=0;i<54;++i){
		if(allcards[i]){
			othersCards[card2level(i)]++;
			remainCards[i] = 1;
		}
	}
	initOpponentModel(pDdz);
}

//信息转换函数，用于衔接Botzone函数和Ddz平台变量
void infoConvert( Ddz * pDdz ){
	setOthersCards( pDdz ); //记牌器，余牌统计
	SortById(pDdz->iOnHand);//信息转换infoConvert
	DirConvertPara = (int)( pDdz->cLandlord - 'A' );//位置矫正参数
	switch((int)(pDdz->cDir-pDdz->cLandlord))
	{
	    case 0:myPosition=0;break;
        case 1:myPosition=1;break;
        case 2:myPosition=2;break;
        case -1:myPosition=2;break;
        case -2:myPosition=1;break;
	}
	//myPosition = abs((int)( pDdz->cDir - 'A' ) - DirConvertPara) % 3;//我的身份（FarmerAB or Landlord）
	lastPlayer = ( myPosition + 2 - pDdz->iLastPassCount ) % 3; //最后出牌人的数字身份（FarmerAB or Landlord）
	lastTurn = -1;
	for(int i=pDdz->iOTmax;i>=0;i--){
		if(pDdz->iOnTable[i][0] >= 0){
			lastTurn = i;
			break;
		}
	}
	lastValidCombo = CardCombo(); //定位上个出牌人历史记录所在的轮数
	if(lastTurn >= 0){ //有前家
		vector<Card> lastCards;
		for(int i=0;pDdz->iOnTable[lastTurn][i]>=0;i++){
			lastCards.push_back(pDdz->iOnTable[lastTurn][i]);
		}
		lastValidCombo = CardCombo( lastCards.begin(), lastCards.end() );
	}
	rebuildOpponentModel(pDdz);
}

//手牌移除函数，vector版HelpTakeOff
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
	// 特判：空
	if (begin == end)
	{
		comboType = CardComboType::PASS;
		return;
	}

	// 每种牌有多少个
	short counts[MAX_LEVEL + 1] = {};

	// 同种牌的张数（有多少个单张、对子、三条、四条）
	short countOfCount[5] = {};

	cards = vector<Card>(begin, end);
	for (Card c : cards)//用桶排还原牌数
		counts[card2level(c)]++;
	for (Level l = 0; l <= MAX_LEVEL; l++)
		if (counts[l])
		{
			packs.push_back(CardPack{ l, counts[l] });//由小到大，比如(0【3】,2)(1【2】,1)(3【5】,3)...
			countOfCount[counts[l]]++;
		}
	sort(packs.begin(), packs.end());//从大到小

	// 用最多的那种牌总是可以比较大小的
	comboLevel = packs[0].level;

	// 计算牌型
	// 按照 同种牌的张数 有几种 进行分类
	vector<int> kindOfCountOfCount;
	for (int i = 0; i <= 4; i++)
		if (countOfCount[i])
			kindOfCountOfCount.push_back(i);
	sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

	int curr, lesser;

	switch (kindOfCountOfCount.size())
	{
	case 1: // 只有一类牌
		curr = countOfCount[kindOfCountOfCount[0]];
		switch (kindOfCountOfCount[0])
		{
		case 1:
			// 只有若干单张
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
			// 只有若干对子
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
			// 只有若干三条
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
			// 只有若干四条
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
	case 2: // 有两类牌
		curr = countOfCount[kindOfCountOfCount[1]];
		lesser = countOfCount[kindOfCountOfCount[0]];
		if (kindOfCountOfCount[1] == 3)
		{
			// 三条带？
			if (kindOfCountOfCount[0] == 1)
			{
				// 三带一
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
				// 三带二
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
			// 四条带？
			if (kindOfCountOfCount[0] == 1)
			{
				// 四条带两只 * n
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
				// 四条带两对 * n
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
float CardCombo::getValue2() const//控手牌权值优化
{
	Level value = this->comboLevel;
    bool biggestS = true;//判断最大是否为单牌
	bool biggestP = true;
	bool biggestT = true;						//判断是否为最大对、三、四
	bool biggestD = true;
	bool DoubleKing = false;//判断是否有双王
	bool FourBoom = false;//判断是否有炸弹

    if((othersCards[13]+othersCards[14])>=2){DoubleKing=true; }//判断双王是否有可能存在他人手牌，如果有，把相应值改为true

    for(Level l=0;l<13;++l)//判断是否有炸弹存在他人手牌
    {
        if(othersCards[l]>3)FourBoom=true;
    }

/***************等待增加一个判断，使其在关键时刻准确判断一个单张是否是控手******************/
	for(Level l=comboLevel+1;l<MAX_LEVEL;++l){	//判断是否为最大单牌
		if(othersCards[l] or (DoubleKing==true) or (FourBoom==true)) biggestS = false;
	}

	for(Level l=comboLevel+1;l<13;++l){
		if(othersCards[l]>1 or (DoubleKing==true) or (FourBoom==true)) biggestP = false;//判断对子
		if(othersCards[l]>2 or (DoubleKing==true) or (FourBoom==true)) biggestT = false;//判断3
		if(othersCards[l]>3 or (DoubleKing==true) or (FourBoom==true)) biggestD = false;//判断4
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
	for(Level l=comboLevel+1;l<12;++l){			//判断是否为最大顺子
		if(othersCards[l] or (DoubleKing==true) or (FourBoom==true)) biggestST = false;
		if(othersCards[l]>1 or (DoubleKing==true) or (FourBoom==true)) biggestST2 = false;
		if(othersCards[l]>2 or (DoubleKing==true) or (FourBoom==true)) biggestPL  = false;

	}
    for(Level l=comboLevel+1;l<13;++l){
        if(othersCards[l]>3 or (DoubleKing==true)) biggestBOOM  = false;
    }
	//得知自己的牌的信息和剩余牌的信息，对权重值进行一些改变
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
	else if(comboType==CardComboType::ROCKET){//王炸
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
float CardCombo::getSeqValue(comboDiv& ComboSeq){ //获取序列总value
	float total = 0;
	for(comboDiv::iterator x = ComboSeq.begin(); x!=ComboSeq.end(); x++){
		CardCombo part = *x;
		total += part.getValue2();
	}
	return SeqDecayRate * total;
}

/*---------------减少重复操作的函数---------------*/
float CardCombo::MergeValue(CardCombo main, comboDiv DivRemain)//三带、四带飞机以及航天飞机的合并
{
	comboDiv singles, pairs;//用来储存单序列
	//short count;//用于计算x带n的数量n
	float Value1=1000, Value2=1000;//用于计算带单或带双价值
	for(comboDiv::iterator k = DivRemain.begin(); k!= DivRemain.end(); k++){//统计单和对儿的数量
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
	if(main.comboType == CardComboType::SSHUTTLE){//航天飞机
		Level mainlv = main.comboLevel;//主牌Level
		Level dlv = main.packs.size(); //主牌序列长度
		if((short)singles.size() >= 2*dlv) {//带2n个单
			Value1 = getExValue(singles, mainlv, dlv, 2);
		}
		if((short)pairs.size() >= 2*dlv) {//带2n个对
			Value2 = getExValue(pairs, mainlv, dlv, 2);
		}
		ComboValue = 4 * roundValWithDecay - minimum(Value1,Value2); //收了四个组合，少了四个回合
	}
	if(main.comboType == CardComboType::PLANE){//飞机
		Level mainlv = main.comboLevel;//主牌Level
		Level dlv = main.packs.size(); //主牌序列长度
		if((short)singles.size() >= dlv) {//带n个单
			Value1 = getExValue(singles, mainlv, dlv, 1);
		}
		if((short)pairs.size() >= dlv) {//带n个对
			Value2 = getExValue(pairs, mainlv, dlv, 1);
		}
		ComboValue = 2 * roundValWithDecay - minimum(Value1,Value2);//收了两个单，少了两个回回合
	}
	if(main.comboType == CardComboType::BOMB){//四带
		Level mainlv = main.comboLevel;//主牌Level
		if((short)singles.size() >= 2) {//带2个单
			Value1 = getExValue(singles, mainlv, 1, 2);
		}
		if((short)pairs.size() >= 2) {//带2个对
			Value2 = getExValue(pairs, mainlv, 1, 2);
		}
		ComboValue = 2 * roundValWithDecay - minimum(Value1,Value2) + mainlv/2 - main.getValue2(); //收了两个组合，少了两个回合 ，没了一个炸弹，多了一组四带二
	}
	if(main.comboType == CardComboType::TRIPLET){//三带
		Level mainlv = main.comboLevel;//主牌Level
		if((short)singles.size() >= 1) {//带1个单
			Value1 = getExValue(singles, mainlv, 1, 1);
		}
		if((short)pairs.size() >= 1) {//带1个对
			Value2 = getExValue(pairs, mainlv, 1, 1);
		}
		ComboValue = roundValWithDecay - minimum(Value1,Value2);//收了1个单，少了1个回合
	}
	ComboValue = maximum( 0 , ComboValue); // 有增益就算进去
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
	ComboDiv TempSeq = div( CardCombo( divcards.begin(),divcards.end() ) ); //承接分割后的有序牌型
	CardCombo CutCombo = CardCombo( cuts.begin(),cuts.end() );
	float ComboValue = TempSeq.Value + CutCombo.getValue2() ;//获得序列的权值
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
		DivRemain.push_back( CutCombo );//把cut的牌型组合入有序牌型
	}
}
//主动出牌部分的余牌权值比较函数，返回值决定是否会更新myAction
bool CardCombo::RemainValueCompare(vector<Card>& remainCards, ComboDiv& FinalSeq, CardCombo& myAction){
	CardCombo remainCombo = CardCombo( remainCards.begin(), remainCards.end());
	ComboDiv tempSeq = remainCombo.div( remainCombo ); //生成余牌序列
	float penalty1 = getComboPenalty(tempSeq.div);
	float penalty2 = getComboPenalty(FinalSeq.div);
	if(tempSeq.Value - penalty1 > FinalSeq.Value - penalty2){//比较与当前最大值的余牌权值大小
		FinalSeq = tempSeq;//取大的
		return true;//“TRUE”：后续会更新myAction
	}
	else return false;
}

float CardCombo::getExValue(comboDiv& Seq, short mainlv, short dlv, short k){
	short count = 0, Value = 0;
	for(auto c = Seq.begin(); c!= Seq.end(); c++){
		Level tmplv = c->comboLevel;
		if(tmplv<=mainlv-dlv or tmplv>mainlv){
			count++;
			Value += c->getValue2();//这个地方其实应该考虑换成getValue2
		}
		if(count >= k*dlv) break;
	}
	if(count < k*dlv) Value = 1000;//废了
	return Value;
}

CardCombo CardCombo::MergeCombo(comboDiv& singles, comboDiv& pairs)//三带、四带飞机以及航天飞机的合并
{
	short SingleValue1 = 100, SingleValue2 = 100, PairValue1 = 100, PairValue2 = 100;//用于计算带单或带双价值
	if(singles.size() >= 1){
		SingleValue1 = (singles.begin())->comboLevel;//带一个单
		if(singles.size() >= 2){
			SingleValue2 = SingleValue1 + (singles.begin()+1)->comboLevel;//带两个单儿
		}
	}
	if(pairs.size() >= 1){
		PairValue1 = (pairs.begin())->comboLevel;
		if(pairs.size() >= 2){
			PairValue2 = PairValue1 + (pairs.begin()+1)->comboLevel;//带两个对儿
		}
	}
	int len = this->packs.size();
	CardCombo tempAction = *this;
	if(this->comboType == CardComboType::PLANE and (PairValue2!=100 or SingleValue2!=100)){//飞机 is Coming
		if(SingleValue2 < PairValue2 and singles.size()>=len){//飞机带小翼
			CardCombo part[5];
			for(int j=0;j<len;j++) part[j] = *(singles.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<len;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=len){//飞机带大翼
			CardCombo part[5];
			for(int j=0;j<len;j++) part[j] = *(pairs.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<len;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
	}
	if(this->comboType==CardComboType::TRIPLET){//三带
		if(SingleValue1 < PairValue1){//三带一
			CardCombo part = *singles.begin();
			vector<Card> ActionCards = this->cards;
			ActionCards.insert( ActionCards.end(), part.cards.begin(), part.cards.end() );
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=1){//三带一
			CardCombo part = *pairs.begin();
			vector<Card> ActionCards = this->cards;
			ActionCards.insert( ActionCards.end(), part.cards.begin(), part.cards.end() );
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
	}
	if(this->comboType==CardComboType::BOMB){//四带
		if(SingleValue2 < PairValue2){//四带一
			CardCombo part[5];
			for(int j=0;j<2;j++) part[j] = *(singles.begin()+j);
			vector<Card> ActionCards = this->cards;
			for(int j=0;j<2;j++){
				ActionCards.insert( ActionCards.end(), part[j].cards.begin(), part[j].cards.end() );
			}
			tempAction = CardCombo( ActionCards.begin(), ActionCards.end() );
		}
		else if(pairs.size()>=2){//四带二
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

/*---------------减少重复操作的函数---------------*/

/**
 * 检查个数最多的CardPack递减了几个
 * 用于计算顺子什么的
 */
int CardCombo::findMaxSeq() const
{
	for (unsigned c = 1; c < packs.size(); c++)
		if (packs[c].count != packs[0].count ||
			packs[c].level != packs[c - 1].level - 1)
			return c;
	return packs.size();
}

Seq CardCombo::findSeq1() const //检测单顺
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//用桶排还原牌数
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=4; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 1)
				break;
		}
		len = start - end;
		if(len >= 5) return Seq{start, len};//0,1,2,3,4共5张牌
	}
	return Seq{12,1};//不行
}
Seq CardCombo::findSeq2() const //检测双顺
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//用桶排还原牌数
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=2; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 2)
				break;
		}
		len = start - end;
		if(len >= 3) return Seq{start, len};
	}
	return Seq{12,1};//不行
}
Seq CardCombo::findSeq3() const //检测飞机
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//用桶排还原牌数
		counts[card2level(c)]++;
	for(start = MAX_STRAIGHT_LEVEL; start>=1; start--){
		for(end = start; end >= 0; end--){
			if(counts[end] < 3)
				break;
		}
		len = start - end;
		if(len >= 2) {return Seq{start, len};}
	}
	return Seq{12,1};//不行
}
Seq CardCombo::findSeq4() const //检测航天飞机
{
	Level start, end, len;
	short counts[MAX_LEVEL + 1] = {};
	for (Card c : cards)//用桶排还原牌数
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
	return Seq{12,1};//不行
}
/*判断指定牌组能否大过当前牌组（这个函数不考虑过牌的情况！）*/
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

//主动出牌函数
CardCombo HelpPass(struct Ddz * pDdz)
{
    //dout<<"HelpPass"<<endl;
	vector<Card> myCards;
	for(int i=0;pDdz->iOnHand[i]!=-1;i++){
		myCards.push_back(pDdz->iOnHand[i]);
	}

	//对手牌进行处理
	CardCombo myCardCombo(myCards.begin(),myCards.end());
	ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
	comboDiv myCardsDiv = myComboDiv.div;

	sort(myCardsDiv.begin(),myCardsDiv.end());

	vector<Card> remainCards;
	CardCombo myAction,tempAction,remainCombo,smallSingle,smallPair;
	ComboDiv tempSeq,FinalSeq;
	comboDiv singles, pairs,straight,plane;//用来储存单序列
	comboDiv normal,king;	// normal 非控手， king 控手
	comboDiv singleCombo,otherCombo,pairCombo;
	float SingleValue1 = 100, SingleValue2 = 100, PairValue1 = 100, PairValue2 = 100;//用于计算带单或带双价值
	for(vector<CardCombo>::iterator k = myCardsDiv.begin(); k!= myCardsDiv.end(); k++){//统计单和对儿的数量
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
		SingleValue1 = (singles.begin())->comboLevel;//带一个单
		if(singles.size() >= 2){
			SingleValue2 = SingleValue1 + (singles.begin()+1)->comboLevel;//带两个单儿
		}
	}
	if(pairs.size() >= 1){
		PairValue1 = (pairs.begin())->comboLevel;
		if(pairs.size() >= 2){
			PairValue2 = PairValue1 + (pairs.begin()+1)->comboLevel;//带两个对儿
		}
	}

	//稳赢残局判据
	int num_normals = normal.size();
	int kicker_remain = singles.size() + pairs.size();
	for(int j = 0; j < king.size(); j++){
		if(king[j].comboType == CardComboType::SINGLE || king[j].comboType == CardComboType::PAIR){
			kicker_remain -= 1;					//非控手里面的单牌和对子有几套
		}
	}
	int accumulate = 0;
	for(vector<CardCombo>::iterator part = myCardsDiv.begin(); part!= myCardsDiv.end(); part++){
		if(part->comboType == CardComboType::TRIPLET){
			if(kicker_remain > 0){
				accumulate += 1;				// 三带可以抵消一张，先累计起来最后再计算
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
	num_normals = num_normals - min(kicker_remain, accumulate);		// 如果带牌足够就减少三条的数量，否则只能把减掉所有带牌的数量

/********LIAO**********/
        if(myCardCombo.comboType != CardComboType::INVALID){ //就一手牌就直接出手了
			if(myCardCombo.packs.size() > 1 && myCardCombo.packs[0].count == 3 && myCardCombo.packs[1].count != 3){
				if(myCardCombo.packs.size() > 2 && (myCardCombo.packs)[1].level == 14 && (myCardCombo.packs)[2].level == 13){			//	看下是不是王炸
					vector<Card> rockets = {card_joker, card_JOKER};
					CardCombo RocketCombo = CardCombo(rockets.begin(), rockets.end());
					return RocketCombo;
				}
			}
		   return myCardCombo;
		}
        if(num_normals<=1 && king.size()>0){//如果只有一张非控手，一直出控手
            //dout<<"KingOut"<<endl;
            sort(king.begin(),king.end());
            CardCombo tempAction = *(king.begin());
            myAction = tempAction.MergeCombo(singles,pairs);
            return myAction;
        }

//对手牌少于这个顺子、飞机的大小或者没有控手，可以先出

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
	{//对手/队友只剩一张牌的保守残局策略
		if(myPosition == Landlord){//地主
		    //dout<<"Landlord:"<<Landlord<<endl;
			if(cardRemaining[FarmerA]==1 ){//如果农民甲只剩一张牌
			    //dout << "LandlordToFarmerA"<<endl;
				/********单牌只带炸弹的情况********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌只有一张且非单牌是一个炸弹
					{
                        return myAction = *(otherCombo.begin());//先把炸弹出了
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌多于一张且非单牌是一个炸弹
					{
                        return myAction = *(singleCombo.rbegin());//先把单牌倒着出
					}

                /********其它的情况********/
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
			if(cardRemaining[FarmerB]==1 ){//如果农民乙只剩一张牌
				/********单牌只带炸弹的情况********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌只有一张且非单牌是一个炸弹
					{
                        return myAction = *(otherCombo.begin());//先把炸弹出了
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌多于一张且非单牌是一个炸弹
					{
                        return myAction = *(singleCombo.rbegin());//先把单牌倒着出
					}
                /********其它的情况********/
                 /*if(singleCombo.begin()->comboLevel>11 and otherCombo.size()==1)//如果剩下的单牌很大，但是非单牌只有一个（其实应该先判断非单排价值)，先出单牌
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
		else if(myPosition == FarmerA){//农民甲
           // dout<<"FarmerA:"<<FarmerA<<endl;
			if(cardRemaining[FarmerB] == 1 && AtoB_dedication==0){//如果下家FarmerB只有一张牌，赶紧让他赢

			    CardCombo Support;
			    Support=*(singleCombo.begin());
				if(singleCombo.size()>0 and Support.getValue2()<7 and Support.comboLevel<11)
                {//有9以下的单牌
                    AtoB_dedication=1;
					return *singleCombo.begin(); //有小单牌出小单（让牌）
                }
				else if(card2level(*myCards.begin())<6)
                    {//有9以下的牌，拆掉自己的combo也要让他
                        AtoB_dedication=1;
                        return CardCombo(myCards.begin(),++myCards.begin());
					}
			}

			else if(cardRemaining[Landlord] == 1){//如果地主只剩一张牌
                //dout<<"FAToFB"<<endl;
				/********单牌只带炸弹的情况********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌只有一张且非单牌是一个炸弹
					{
                        return myAction = *(otherCombo.begin());//先把炸弹出了
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌多于一张且非单牌是一个炸弹
					{
                        return myAction = *(singleCombo.rbegin());//先把单牌倒着出
					}

                /********其它的情况********/
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
		else if(myPosition == FarmerB){//农民乙
			if(cardRemaining[Landlord] == 1){//如果地主只剩一张牌

			    //dout << "FarmerBToLandlord"<<endl;
				/********单牌只带炸弹的情况********/
				if(singleCombo.size()==1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌只有一张且非单牌是一个炸弹
					{
                        return myAction = *(otherCombo.begin());//先把炸弹出了
					}
				else if(singleCombo.size()>1 and otherCombo.size()>0 and ((otherCombo.begin())->comboType == CardComboType::BOMB or
					(otherCombo.begin())->comboType == CardComboType::ROCKET))//若单牌多于一张且非单牌是一个炸弹
					{
                        return myAction = *(singleCombo.rbegin());//先把单牌倒着出
					}

                /********其它的情况********/
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

	/* Endgame: with big single (2/joker/JOKER) + few small singles,
	 * play small first, save the big card for the winning play. */
	if(king.size() == 1 && singles.size() >= 2 && singles.size() <= 3) {
		CardCombo kingCard = king[0];
		if(kingCard.comboType == CardComboType::SINGLE && kingCard.comboLevel >= 12) {
			for(auto it = singles.rbegin(); it != singles.rend(); ++it) {
				if(it->comboLevel < 12) return *it;
			}
			return *(singles.rbegin());
		}
	}

	{ //---------------------权值优先主动出牌---------------------
		{ //---------------------权值优先主动出牌---------------------
		FinalSeq = myCardCombo.div( myCardCombo );//myAction为Pass，所有牌都是余牌，仅初始化
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
		//主动出牌SINGLE&PAIR特判
		if(singles.size()==1 and pairs.size()>1){//只有一个单牌的时候，PAIR多考虑先出对儿（或许管的上）
			tempAction = *(pairs.begin());
		}
		else if(pairs.size()==1 and singles.size()>1){//只有一个对儿的时候，SINGLE多考虑先出单（或许管的上）
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
		myAction = tempAction;		// HelpPass必须出牌，否则直接输
	}
	return myAction;
}

void HelpPla(struct Ddz * pDdz){
	for(int i=0;i<kPlaMax;i++)		//初始化
		for(int j=0;j<21;j++) pDdz->iPlaArr[i][j]=-1;
	pDdz->iPlaCount=0; //出牌可行解数量初始化为0
	if (pDdz->iLastTypeCount==0){
		//连续两家PASS，则进入主动出牌模块
		CardCombo myAction = HelpPass(pDdz); //函数名可以调整
		//这里需要把myAction.cards转化成数组替换iToTable
		int k=0;
		for(auto i=myAction.cards.begin(); i!=myAction.cards.end(); i++){
			pDdz->iPlaArr[pDdz->iPlaCount][k++] = *i;
		}
		pDdz->iPlaArr[pDdz->iPlaCount][k] = -1;
		pDdz->iPlaCount++;		// 主动出牌可行解加1
		return;
	}
	else{
		Rocket(pDdz); //火箭
		Bomb(pDdz); //炸弹
		//上述两者可以压不同类牌
		//被动出牌，牌型必然与要压的牌对应
		if(301 == pDdz->iLastTypeCount)//单张
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


//D01-START计算当前手中余牌估值,预设不拆对牌和连牌，建议进一步自行完善
//最后修订者:谢文&梅险,最后修订时间:15-02-11
double CalCardsValue(int iPlaOnHand[])
{
	double dSum = -10000;			//估值
	//信息转换infoConvert
		SortById(iPlaOnHand);
		vector<Card> myCards;
		for(int i=0;iPlaOnHand[i]!=-1;i++) myCards.push_back(iPlaOnHand[i]);
		//对手牌进行处理
		CardCombo myCardCombo(myCards.begin(),myCards.end());
		ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
		comboDiv myCardsDiv = myComboDiv.div;
		sort(myCardsDiv.begin(),myCardsDiv.end());
	dSum = myComboDiv.Value - roundValWithDecay*(short)myCardsDiv.size();
	return dSum;
}
//D01-END
//I02-END
//I02-START计算己方叫牌策略:预设3分或0分，建议进一步自行完善
//最后修订者:梅险,最后修订时间:15-02-12
int CalBid(struct Ddz * pDdz	)
{
	int i;
	int iMyBid=3;			//叫牌
	setOthersCards( pDdz ); //记牌器，余牌统计
	double value = CalCardsValue(pDdz->iOnHand);
	value = minimum(value, -20); //value常负
	iMyBid += (value+25)/20;
	iMyBid = maximum(iMyBid, 0);
	//cout << value <<endl;
	//cout << iMyBid<<endl;
	for(i=0;i<3;i++)
		if(pDdz->iBid[i]>=iMyBid)
			iMyBid=0;
    if(iMyBid==3)dout<<"value3:"<<value<<endl;
    if(iMyBid==2)dout<<"value2:"<<value<<endl;
    if(iMyBid==1)dout<<"value1:"<<value<<endl;
    if(iMyBid==0)dout<<"value0:"<<value<<endl;
	return iMyBid;
}

/*
	以下是神经网络构建特征部分
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
	if(_len[localCount] >= limit[localCount]){			// localLevel 前 _len 个等级组成顺子
		if(localCount == 3){	//有三顺
			if(countOfCount[2] >= 2){	//存在对子，可以组飞机带对
				for(int k = 0; k < _len[localCount]; k++){
					coord = coordLevelTransform(localLevel - k, z, 12);		//飞机位置置1
					feature[coord] = 1;
				}
			}
			if(countOfCount[1] >= 2){	//存在单牌，可以组飞机带单
				for(int k = 0; k < _len[localCount]; k++){
					coord = coordLevelTransform(localLevel - k, z, 11);		//飞机位置置1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 4);		//三顺位置置1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 3);		//双顺位置置1
					feature[coord] = 1;
					coord = coordLevelTransform(localLevel - k, z, 2);		//由于双顺也可以组成单顺，所以单顺位置也置1
					feature[coord] = 1;
				}
			}
		}
		else if(localCount == 2){	//有双顺
			int coord;
			for(int k = 0; k < _len[localCount]; k++){
				coord = coordLevelTransform(localLevel - k, z, 3);		//双顺位置置1
				feature[coord] = 1;
				coord = coordLevelTransform(localLevel - k, z, 2);		//由于双顺也可以组成单顺，所以单顺位置也置1
				feature[coord] = 1;
			}
		}
		else{	//有顺子
			int coord;
			for(int k = 0; k < _len[localCount]; k++){
				coord = coordLevelTransform(localLevel - k, z, 2);
				feature[coord] = 1;
			}
		}
	}
}

void add_feature(vector<int>& cardsInThisTurn, int z, int layer){//添加手牌特征
	CardCombo localCardCombo = CardCombo(cardsInThisTurn.begin(), cardsInThisTurn.end());
	if(layer == 1){		//构建第一层手牌特征
		auto &localPack = localCardCombo.packs;
		vector<Card> &localCard = localCardCombo.cards;

		short * counts = new short[MAX_LEVEL + 1];
		short * countOfCount = new short[5];
		memset(counts, 0, (MAX_LEVEL + 1) * sizeof(short));
		memset(countOfCount, 0, 5 * sizeof(short));

		for (Card c : localCard)//用桶排还原牌数
			counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
				countOfCount[counts[l]]++;

		for(int i = 0; i < localPack.size(); i++){
			auto item = localPack[i];
			int coord;
			if(item.count == 4){
				Level localLevel = item.level;
				if(countOfCount[2] > 0){	//能组成四带二
					coord = coordLevelTransform(localLevel, z, 8);
					feature[coord] = 1;
				}
				if(countOfCount[1] > 0){	//能组成四带一
					coord = coordLevelTransform(localLevel, z, 7);
					feature[coord] = 1;
				}
				coord = coordLevelTransform(localLevel, z, 6);	//添加炸弹
				feature[coord] = 1;
			}
			else if(item.count == 3){
				Level localLevel = item.level;
				if(countOfCount[2] > 0){	//有三带二
					coord = coordLevelTransform(localLevel, z, 10);	//添加三带二
					feature[coord] = 1;
				}
				if(countOfCount[1] > 0){	//有三带一
					coord = coordLevelTransform(localLevel, z, 9);	//添加三带一
					feature[coord] = 1;
				}
				coord = coordLevelTransform(localLevel, z, 5);	//添加三张
				feature[coord] = 1;
			}
			else if(item.count == 2){
				Level localLevel = item.level;
				coord = coordLevelTransform(localLevel, z, 1);	//添加对子
				feature[coord] = 1;
			}
			else if(item.count == 1){
				Level localLevel = item.level;
				coord = coordLevelTransform(localLevel, z, 0);	//添加单张
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
		if(prevLevel != -1){		//末尾检查
			checkSeq(len_vec, limit, localCardCombo, localPack.size()-1, counts, countOfCount, z);
		}
		delete [] counts;
		delete [] countOfCount;
	}
	else{				//构建第三层出牌特征
		CardComboType localType = localCardCombo.comboType;
		auto &curPack = localCardCombo.packs;
		int coord;
		if(curPack.size() > 0){
			if(curPack[0].count == 4){
				if(curPack[curPack.size()-1].count == 4){		//炸弹
					coord = coordLevelTransform(curPack[0].level, z, 6);
					feature[coord] = 1;
				}
				else if(curPack[curPack.size()-1].count == 2){	//四带一
					coord = coordLevelTransform(curPack[0].level, z, 7);
					feature[coord] = 1;
				}
				else{	//四带二
					coord = coordLevelTransform(curPack[0].level, z, 8);
					feature[coord] = 1;
				}
			}
			else if(curPack[0].count == 3){			//以下三项可以合并，有时间回头来改
				int coord;
				if(localType == CardComboType::TRIPLET){				//三条
					coord = coordLevelTransform(curPack[0].level, z, 5);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::TRIPLET1){			//三带一
					coord = coordLevelTransform(curPack[0].level, z, 9);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::TRIPLET2){			//三带二
					coord = coordLevelTransform(curPack[0].level, z, 10);
					feature[coord] = 1;
				}
				else if(localType == CardComboType::PLANE){				//飞机
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 4);
						feature[coord] = 1;
						if(card.count != 3){
							break;
						}
					}
				}
				else if(localType == CardComboType::PLANE1){			//飞机带单
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 11);
						feature[coord] = 1;
						if(card.count != 3){
							break;
						}
					}
				}
				else if(localType == CardComboType::PLANE2){			//飞机带双
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
				if(localType == CardComboType::PAIR){		//对子
					coord = coordLevelTransform(curPack[0].level, z, 1);
					feature[coord] = 1;
				}
				else{										//双顺
					for(auto card : curPack){
						coord = coordLevelTransform(card.level, z, 3);
						feature[coord] = 1;
					}
				}
			}
			else{
				int coord;
				if(localType == CardComboType::SINGLE){		//单张
					coord = coordLevelTransform(curPack[0].level, z, 0);
					feature[coord] = 1;
				}
				else{										//单顺
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
	//第一层特征
	vector<int> cardOnHand;
	int hand_count = 0;
	for(int i = 0; pDdz->iOnHand[i] >= 0; i++){
		cardOnHand.push_back(pDdz->iOnHand[i]);
		int coord = coordTransform(pDdz->iOnHand[i], 0);
		feature[coord] = 1;
		hand_count += 1;
	}
	add_feature(cardOnHand, 0, 1);		//添加手牌特征

	vector<int>().swap(cardOnHand);			//清理变量

	//添加玩家位置
	int selfPos = abs((int)(pDdz->cDir - pDdz->cLandlord));
	feature[dim1 * 0 + dim2 * 17 + 10] = selfPos;
	//添加剩余手牌
	feature[dim1 * 0 + dim2 * 17 + 14] = hand_count;

	//第二层特征
	for(int index = 0; index < 54; index++){
		if(remainCards[index]){
			int coord = coordTransform(index, 1);
			feature[coord] = 1;
		}
	}
	//添加玩家位置
	feature[dim1 * 1 + dim2 * 17 + 10] = selfPos;

	//第三层特征
	vector<vector<int> > prev6;		//前六(18手)手牌

	int num_play = pDdz->iOTmax;
	int count_in = 0;

	//初始化剩余手牌数量
	vector<int>* handCardRemain = new vector<int>(3, 0);
	for(int i = 0; i < 3; i++){
		handCardRemain->at(i) = cardRemaining[i];
	}

	while(num_play > 0 && count_in < 18){		//6轮共18手
		int cur_turn = count_in / 3;
		vector<int> cardsInThisTurn;
		for(int i = 0; pDdz->iOnTable[num_play][i] >= 0; i++){
			cardsInThisTurn.push_back(pDdz->iOnTable[num_play][i]);
			int coord = coordTransform(pDdz->iOnTable[num_play][i], count_in + 2);
			feature[coord] = 1;
		}
		add_feature(cardsInThisTurn, count_in + 2, 3);
		prev6.push_back(cardsInThisTurn);

		//添加轮次，玩家编号和剩余手牌
		feature[2 + dim1 * count_in + dim2 * 17] = cur_turn;
		int player = ((myPosition + 3) - (count_in % 3)) % 3;
		feature[2 + dim1 * count_in + dim2 * 17 + 10] = player;
		feature[2 + dim1 * count_in + dim2 * 17 + 14] = handCardRemain->at(player);
		handCardRemain->at(player) += cardsInThisTurn.size();
		num_play--;
		count_in++;
		vector<int>().swap(cardsInThisTurn);
	}

	//第四层特征
	feature[dim1 * 20 + dim2 * 17 + 10] = selfPos;
	for(int x = 0; x < prev6.size(); x++){
		for(int y = 0; y < prev6[x].size(); y++){
			int coord = coordTransform(prev6[x][y], 20);
			feature[coord] = 1;
		}
	}
	vector<vector<int> >().swap(prev6);			//清理变量
	vector<int>().swap(cardOnHand);
}



/********LIAO**********/
//P030601-START计算己方出牌策略
//最后修订者:夏侯有杰&梅险,最后修订时间:15-02-12
void CalPla(struct Ddz * pDdz)
{
	int i;
	double dValueNow;
	double dValueMax=-9999;
	int iMax = 0;
	infoConvert( pDdz );

	HelpPla(pDdz);				//主要计算推荐出牌pDdz->iPlaArr[],pDdz->iPlaCount
	for(i=0;i<pDdz->iPlaCount;i++)
	{
		vector<Card> Actions;
		for (int j = 0;pDdz->iPlaArr[i][j] >= 0; j++)
			Actions.push_back(pDdz->iPlaArr[i][j]);
		CardCombo combo(Actions.begin(), Actions.end());
		HelpTakeOff(pDdz,i);	//假设取走了第i组牌，将剩余的牌放入pDdz->iPlaOnHand[]
		//dout << i <<" : "<<print(combo)<<'\t';
		dValueNow = CalCardsValue(pDdz->iPlaOnHand);			//计算余牌估值
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
		//对手牌进行处理
		vector<Card> myCards,remainCards;
		for(i=0;pDdz->iOnHand[i]!=-1;i++) myCards.push_back(pDdz->iOnHand[i]);
		CardCombo myCardCombo(myCards.begin(),myCards.end());
		ComboDiv myComboDiv = myCardCombo.div(myCardCombo);
		comboDiv myCardsDiv = myComboDiv.div;
		sort(myCardsDiv.begin(),myCardsDiv.end());
		////dout << "Enter into Special Judge:"<<endl;

		iCanWin = false;
		comboDiv normal,king;//控手牌信息（稳赢残局）
		sort(myCards.begin(),myCards.end());
		removeCards(myCards, myAction.cards, remainCards);
		CardCombo remainCombo = CardCombo(remainCards.begin(), remainCards.end());
		ComboDiv remainDiv = remainCombo.div(remainCombo);
		comboDiv remaindiv = remainDiv.div;

		if(myCardCombo.packs.size() > 1 && myCardCombo.packs[0].count == 3 && myCardCombo.packs[1].count != 3){
			if(myCardCombo.packs.size() > 2 && (myCardCombo.packs)[1].level == 14 && (myCardCombo.packs)[2].level == 13){			//	看下是不是王炸
				pDdz->iToTable[0] = card_joker;
				pDdz->iToTable[1] = card_JOKER;
				pDdz->iToTable[2] = -1;
				return;

			}
		}
		comboDiv singles, pairs;
		for(vector<CardCombo>::iterator k = myCardsDiv.begin(); k!= myCardsDiv.end(); k++){//是否能稳赢
			CardCombo part = *k;
			if(part.getValue2()>=roundValWithDecay)
				king.push_back(part);
			else normal.push_back(part);
            if(part.comboType == CardComboType::SINGLE)
                singles.push_back(part);
            if(part.comboType == CardComboType::PAIR)
                pairs.push_back(part);
		}
			//稳赢残局判据
	int num_normals = normal.size();
	int kicker_remain = singles.size() + pairs.size();

	int small_normals=singles.size() + pairs.size();
	for(vector<CardCombo>::iterator k = singles.begin(); k!= singles.end(); k++){//是否能稳赢
			CardCombo part = *k;
			if(part.comboLevel>10)//大于K的单牌，  2
				small_normals--;
            else if(part.getValue2()>=7)
                small_normals--;
		}
    for(vector<CardCombo>::iterator k = pairs.begin(); k!= pairs.end(); k++){//是否能稳赢
			CardCombo part = *k;
			if(part.comboLevel>9)//大于Q的对牌
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
			kicker_remain -= 1;					//非控手里面的单牌和对子有几套
		}
	}
	int accumulate = 0;
	for(vector<CardCombo>::iterator part = myCardsDiv.begin(); part!= myCardsDiv.end(); part++){
		if(part->comboType == CardComboType::TRIPLET){
			if(kicker_remain > 0){
				accumulate += 1;				// 三带可以抵消一张，先累计起来最后再计算
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
	num_normals = num_normals - min(kicker_remain, accumulate);		// 如果带牌足够就减少三条的数量，否则只能把减掉所有带牌的数量

	//dout<<"I am "<< pDdz->cDir<<" num_normals: "<<num_normals<<" accumulate: "<<accumulate<<"  "<<print(king)<<"  "<<print(normal)<<endl;


		//就一手牌就直接出手了
		if(remainCombo.comboType == CardComboType::PASS){
			 iCanWin = true;
		}
		//如果只有一张非控手，一直出控手能稳赢
		if(num_normals<=1 && king.size()>0){
			iCanWin = true;
		}

/***************等待增加一个判断，使其合理憋牌******************/
	if(pDdz->iLastTypeCount!=0 and pDdz->iPlaCount>0){//被动出牌的特判
		if(iCanWin && remainCombo.comboType != CardComboType::PASS){			//如果稳赢了，尝试直接出控手夺取胜利果实
			sort(king.begin(),king.end());
			for(int k = king.size()-1; k >= 0; k--){
				if(lastValidCombo.canBeBeatenBy(king[k]))		//出最小的控手
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
		}				// 如果控手都出不起，那只能出normal牌了
		if(!iCanWin){
/***************Liao↓*****************/
            bool bigSingle=false;//用来判断上家队友打牌的大小决定是否让牌
            bool bigPair=false;//用来判断上家队友打牌的大小决定是否让牌
            bool mybigSingle=false;//用来判断自己打牌的大小决定是否让牌
            bool mybigPair=false;//用来判断自己打牌的大小决定是否让牌
			//单张没必要用火箭管，留一个大王更好打
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

			if(myPosition == FarmerB)//如果是农民乙，要合理让牌
			{
			    //if(myAction.comboType==CardComboType::BOMB){dout<<"small_normals:"<<small_normals<<endl;}
				if(lastPlayer == FarmerA){
					if(cardRemaining[Landlord] == 1){
                        if(lastValidCombo.getValue2()>=7){
                            pDdz->iToTable[0] = -1;//让牌
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

					/* Cooperate: pass only when teammate winning (<=2 cards) or their
					 * play is strong (value>=7) and we have many small cards (>3). */
					bool tmWin = (cardRemaining[FarmerA] <= 2);
					if(tmWin and num_normals > 1){ pDdz->iToTable[0] = -1; return; }
					if(lastValidCombo.getValue2() >= 7 and num_normals > 3){ pDdz->iToTable[0] = -1; return; }
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
					/***********对弈地主时的让牌***********/
					//自己的牌很大，地主剩余牌数大于某值，自己散的小的牌很多，作为农民乙的让牌
					//检测自己散牌小牌数量多少

					if(((myAction.comboLevel>=11 or myAction.comboType==CardComboType::BOMB) or myAction.comboType==CardComboType::BOMB)and cardRemaining[Landlord]>=10 and small_normals>=4 and
                        (myAction.comboType==CardComboType::PAIR or myAction.comboType==CardComboType::SINGLE
                         or myAction.comboType==CardComboType::TRIPLET or myAction.comboType==CardComboType::TRIPLET1
                         or myAction.comboType==CardComboType::TRIPLET2 or myAction.comboType==CardComboType::BOMB
                         or myAction.comboType==CardComboType::ROCKET)){//自己的牌很大，地主留的牌很多，自己散牌很多时(除顺子这些长牌外，这些牌一定会顶牌)
                        pDdz->iToTable[0] = -1;//让牌
						 return;
					}


				}
			}
			if (myPosition == FarmerA)//如果是农民甲，只要别恶性内斗就行（K为界限）
			{
				if(lastPlayer == FarmerB){
					/* Cooperate: pass only when teammate winning (<=2 cards) or their
					 * play is strong (value>=7) and we have many small cards (>3). */
					bool tmWin = (cardRemaining[FarmerB] <= 2);
					if(tmWin and num_normals > 1){ pDdz->iToTable[0] = -1; return; }
					if(lastValidCombo.getValue2() >= 7 and num_normals > 3){ pDdz->iToTable[0] = -1; return; }
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
					/***********对弈地主时的让牌***********/
					/* vs Landlord: only pass in very early game (>=13 cards left,
					 * >=6 small cards) to save big cards for later. */
					if(myAction.comboLevel>=11 and cardRemaining[Landlord]>=13 and small_normals>=6){
						pDdz->iToTable[0] = -1; return;
					}

				}
			}
			if(myPosition == Landlord){
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
                    pDdz->iToTable[0] = -1;//让牌
                        return;
				}*/

				/***********对弈农民时的让牌***********/
                if(myAction.comboType==CardComboType::BOMB)dout<<"small_normals:"<<small_normals<<endl;
					/* Landlord: only pass in very early game, both farmers >=10 cards,
					 * >=6 small cards, to save big cards for later. */
					if(myAction.comboLevel>=11 and cardRemaining[FarmerA]>=10 and cardRemaining[FarmerB]>=10 and small_normals>=6){
						pDdz->iToTable[0] = -1; return;
					}

			}
		}
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
	以下是模拟对局部分
*/

//模拟一次出手
void Round(struct Ddz * pDdz, string& pCmd, vector<string>& answer){
		InputMsg(pDdz, pCmd, true);			  //输入信息
		AnalyzeMsg(pDdz);		          //分析处理信息
		OutputMsg(pDdz, answer, true);		  //输出信息
		//CalOthers(pDdz);		          //计算其它数据
}

//叫牌结果判定
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
		currentPlayer = maxBider.first - 'A';		//更新当前动作者，叫到地主的人先出牌
	}
	else{
		currentPlayer = (currentPlayer + 1) % 3;	//上家叫完轮到下家叫
		pCmd = "BID WHAT";
	}
}

//判断游戏是否结束
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

//分牌
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

//发牌
void DeliverCards(string& pCmd, vector<string>& deliveredCards, vector<string>& answer){
	int i = (answer.size() - 3) % 3;
	pCmd = deliveredCards[i];
}

//根据玩家回复信息更新平台的指示命令
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
						currentPlayer = 0;		//初始阶段不轮流操作
						break;
				case 'O':	//OK
						switch(s[3]){
							case 'I':		// ok info
									DeliverCards(pCmd, deliveredCards, answer);
									currentPlayer = 0;		//初始阶段不轮流操作
									break;
							case 'D':
								    pCmd = "BID WHAT";
									currentPlayer = 0;		//初始阶段不轮流操作
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

//开局阶段，初始化全局变量
void InitParams(){
	othersCards.assign(zeros.begin(), zeros.end());
	memset(cardRemaining, 0, 3 * sizeof(int));
	DirConvertPara = 0;
	myPosition = 0;
	lastPlayer = 0;
	lastTurn = 0;
	iCanWin = false;
}

//模拟一局比赛
int Play(){
	string pCmd;				//模拟平台给的命令
	vector<string>().swap(answer);		//玩家回应信息集合
	currentPlayer = 0;			//初始化动作者为A
	int Winner = -1;

	pair<char,char> maxBider = make_pair('A', '0');

	// struct Ddz tDdz1, *pDdz1=&tDdz1;
	// struct Ddz tDdz2, *pDdz2=&tDdz2;
	// struct Ddz tDdz3, *pDdz3=&tDdz3;
	struct Ddz *pDdz1 = new Ddz;
	struct Ddz *pDdz2 = new Ddz;
	struct Ddz *pDdz3 = new Ddz;
	InitTurn(pDdz1);			//初始化数据
	InitTurn(pDdz2);			//初始化数据
	InitTurn(pDdz3);			//初始化数据
	InitParams();				//初始化全局变量

	buildDeliveredCards(deliveredCards);

	pCmd = "DOUDIZHUVER 1.0";
	vector<struct Ddz *> pDdzVector;
	pDdzVector.push_back(pDdz1);
	pDdzVector.push_back(pDdz2);
	pDdzVector.push_back(pDdz3);

	int exitPlayer = -1;		//胜负标志
	int roll = 0;				//出手次数统计变量
	while(1){
		for(int i = 0; i < 3; i++){				//模拟一次出牌
			int index = (currentPlayer + i) % 3;
			//玩家1采用训练权重，玩家2，3采用原始权重
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
			if(lastCommand[n-2] == '-'){						//检查pass
		        string prev = *(answer.end()-2);
				if(prev[3] == 'L'){			// 开头pass
					return 1;
				}
				string tmp1 = *(answer.end() - 4);	//连续pass
				string tmp2 = *(answer.end() - 7);
				if(tmp1[tmp1.length()-2] == '-' && tmp2[tmp2.length()-2] == '-'){
					return 1;	//默认B胜利，只要不是A就行
				}
			}

			assert(pDdzVector[0]->iOnHand[0] < 60);
			assert(pDdzVector[1]->iOnHand[0] < 60);
			assert(pDdzVector[2]->iOnHand[0] < 60);

			platformAction(pCmd, answer, deliveredCards,pDdzVector[0], pDdzVector[1], pDdzVector[2], maxBider);		//更新平台命令
		}
		if(exitPlayer == 1)				//决出胜负则退出循环
			break;
		roll++;
	}
	delete pDdz1;
	delete pDdz2;
	delete pDdz3;
	return Winner;
}

/*
	以上是模拟对局部分
*/



int main()
{
	clock_t startTime, endTime;
	int totalRounds = 1200;
	int winTimes = 0;
	int winner;
	bool selfPlay = false;		// 是否自我对局:改为false则和原来的程序一样
	if(selfPlay){
		startTime = clock();

		// 从txt读取参数
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
				srand((unsigned)time(NULL));	//刷新随机数种子
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
		InitTurn(pDdz);			//初始化数据
		while(pDdz->iStatus!=0)
		{
			InputMsg(pDdz, " ", false);			//输入信息
			AnalyzeMsg(pDdz);		//分析处理信息
			vector<string> tmp;
			OutputMsg(pDdz, tmp, false);		//输出信息
			CalOthers(pDdz);		//计算其它数据
		}
		dout.close();
	}
	return 0;
}
