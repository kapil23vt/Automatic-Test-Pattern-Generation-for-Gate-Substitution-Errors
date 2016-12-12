#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;

//only the required values are kept 
#define ALLONES 0xffffffff
#define MAXlevels 10000
#define MAXIOS 5120
#define MAXFanout 10192
#define MAXFFS 40048
#define MAgatenumberS 100000
#define MAXevents 100000

//Enumerating all the gate types from lsim
enum
{
    JUNK,           /* 0 */
    T_input,        /* 1 */
    T_output,       /* 2 */
    T_xor,          /* 3 */
    T_xnor,         /* 4 */
    T_dff,          /* 5 */
    T_and,          /* 6 */
    T_nand,         /* 7 */
    T_or,           /* 8 */
    T_nor,          /* 9 */
    T_not,          /* 10 */
    T_buf,          /* 11 */
    T_tie1,         /* 12 */
    T_tie0,         /* 13 */
    T_tieX,         /* 14 */ 
    T_tieZ,         /* 15 */    
};

class gateLevelCkt
{
    int numgates;               // total number of gates (faulty included)
    int maxlevels;              // number of levels in gate level ckt
    int numInputs;              // number of PIs
    int numOutputs;              // number of POs
    int maxLevelSize;           // maximum number of gates in one given level
    int inputs[MAXIOS];
    int outputs[MAXIOS];
    
    unsigned char *gtype;                 // gate type
    short *fanin;               // number of fanins of gate
    short *fanout;				// number of fanouts of gate
    int *levelNum;              // level number of gate
    int **faninlist;            // fanin list of gate
    int **fanoutlist;           // fanout list of gate
    
    //added from fsim.cpp 
    char *sched;                // scheduled on the wheel yet?
    unsigned int *value1;
    unsigned int *value2;       // value of gate
	unsigned int *faultValue1;
    unsigned int *faultValue2;  // faulty value of gate
    
    //Adding variable for the Gate Substitution Error
    int *faultyGate;			
    int *gateReplacerType;			
    bool *stuckAtFault;          
    int *originalGateType;      
    int *dFrontier;            
    
	// for simulator
    int **levelEvents;          // event list for each level in the circuit
    int *levelLen;              // event list length
    int numlevels;              // total number of levels in wheel
    int currLevel;              // current level
    int *activation;            // activation list for the current level in circuit
    int actLen;                 // length of the activation list
    int *actFFList;             // activation list for the FF's
    int actFFLen;               // length of the actFFList

public:
	gateLevelCkt(char *);     // constructor

	//simulator information
    void setupWheel(int, int);
    void insertEvent(int, int);
    int  retrieveEvent();
    void goodsim();             // logic sim (no faults inserted)
    void faultsim();              	// Faulty simulation
 
	//PODEM functions
    void podemFunction(); //PODEM Function
    bool recursivePodem(int,bool);// Recursive PODEM function
    void getObjective(int,bool);// Get Objective function
    bool backTrace(int,bool);// Back-trace function
    bool xPathCheck (int);// Function to check if there exists a x-path from D-frontier to PO
    void setDontCare();//Function to set all the primary input values to don't care     
    
    //Getting the D-frontier for the given gate
    void getdFrontier(int);   
    
    //Faulty Gate Insertion
    void faultSetup(); //Each gate will be loaded with s-a-0 and s-a-1 fault, faulty gate and its replacement is also considered

	//Faulty Gate Substitution
    void gateReplacer(int,int);//Replaces the gate to insert fault in the circuit.
    bool faultExcited(int);//to make sure that fault is getting excited
};

int OBSERVE, INIT0; //from fsim
gateLevelCkt *circuit; 

int numberOfFaults=0;
int myObjective;
int backTraceGate;// Variable to store the gate number of the backtraced gate
int dfront;//Counter to store the gate numbers in the dFrontier
int faultCounter; //Variable to iterate through each fault
int faultsDetected=0; //Variable to store the number of faults detected by the PODEM function

bool result = false;//result is True when PODEM detects the fault
bool pathFound=false;//pathFound is true when the fault can be excited
bool objValue;//Boolean value of myObjective
bool flag=true;//flag = true implies success in PODEM algorithm

inline void gateLevelCkt::insertEvent(int levelN, int gateN) //fsim
{

    levelEvents[levelN][levelLen[levelN]] = gateN;
    levelLen[levelN]++;
}

// constructor: reads in the *.lev file for the gate-level ckt
gateLevelCkt::gateLevelCkt(char *cktName)
{
    FILE *yyin; //fsim
    char fName[256]; //fsim
    int i, j, count; //fsim
    char c; //fsim
    int netnum, junk; //fsim
    int f1, f2, f3; //fsim
    int levelSize[MAXlevels]; //fsim
    strcpy(fName, cktName); //fName = c17
    strcat(fName, ".lev"); //fName = c17.lev
    yyin = fopen(fName, "r"); //opening c17.lev in read mode
    if (yyin == NULL)
    {
        fprintf(stderr, "Can't open .lev file\n");
        exit(-1);
    }

    numInputs = numgates = numOutputs = maxlevels = 0;//fsim
    maxLevelSize = 32;//fsim
    for (i=0; i<MAXlevels; i++) //fsim
        levelSize[i] = 0;

    fscanf(yyin, "%d", &count); // number of gates
    fscanf(yyin, "%d", &junk);

    // allocate space for gates
    gtype = new unsigned char[count+64];//fsim all
    fanin = new short[count+64];
    fanout = new short[count+64];
    levelNum = new int[count+64];
    faninlist = new int * [count+64];
    fanoutlist = new int * [count+64];
    sched = new char[count+64];
    value1 = new unsigned int[count+64];
    value2 = new unsigned int[count+64];
    faultValue1 = new unsigned int[count+64];
    faultValue2 = new unsigned int[count+64];
    
    dFrontier = new int[count]; //???

    for (i=1; i<count; i++)
    {
        fscanf(yyin, "%d", &netnum);
        fscanf(yyin, "%d", &f1);
        fscanf(yyin, "%d", &f2);
        fscanf(yyin, "%d", &f3);

        numgates++;
        gtype[netnum] = (unsigned char) f1;
        f2 = (int) f2;
        levelNum[netnum] = f2;
        levelSize[f2]++;

        if (f2 >= (maxlevels))
            maxlevels = f2 + 5;
        if (maxlevels > MAXlevels)
        {
            fprintf(stderr, "MAXIMUM level (%d) exceeded.\n", maxlevels);
            exit(-1);
        }

        fanin[netnum] = (int) f3;
        if (f3 > MAXFanout)
            fprintf(stderr, "Fanin count (%d) exceeded\n", fanin[netnum]);

        if (gtype[netnum] == T_input)
        {
            inputs[numInputs] = netnum;
            numInputs++;
        }
		sched[netnum] = 0;
        faninlist[netnum] = new int[fanin[netnum]];
        
        for (j=0; j<fanin[netnum]; j++)
        {
            fscanf(yyin, "%d", &f1);
            faninlist[netnum][j] = (int) f1;
        }

        for (j=0; j<fanin[netnum]; j++) 
        fscanf(yyin, "%d", &f1);
		fscanf(yyin, "%d", &f1);
        fanout[netnum] = (int) f1;
        
        if (gtype[netnum] == T_output)
        {
            outputs[numOutputs] = netnum;
            numOutputs++;
        }
        
        if (fanout[netnum] > MAXFanout)
            fprintf(stderr, "Fanout count (%d) exceeded\n", fanout[netnum]);

        fanoutlist[netnum] = new int[fanout[netnum]];
        
        for (j=0; j<fanout[netnum]; j++)
        {
            fscanf(yyin, "%d", &f1);
            fanoutlist[netnum][j] = (int) f1;
        }
        while ((c = getc(yyin)) != '\n' && c != EOF)
            ;
    }  // for (i...) 

    fclose(yyin);
    numgates++;
    for (i=0; i<maxlevels; i++)
    {
        if (levelSize[i] > maxLevelSize)
            maxLevelSize = levelSize[i] + 1;
    }
    
    for (i=numgates; i<numgates+64; i++)
    {
	value1[i] = faultValue1[i] = 0;
	value2[i] = faultValue2[i] = ALLONES;
    }
    
    printf("Successfully read in circuit:\n");
    printf("\t%d PIs.\n", numInputs);
    printf("\t%d POs.\n", numOutputs);
    printf("\t%d total number of gates\n", numgates);
    printf("\t%d levels in the circuit.\n", maxlevels / 5);
    
	setupWheel(maxlevels, maxLevelSize); //fsim
}

void gateLevelCkt::setupWheel(int numLevels, int levelSize)//fsim
{
    int i;

    numlevels = numLevels;
    levelLen = new int[numLevels];
    levelEvents = new int * [numLevels];
    for (i=0; i < numLevels; i++)
    {
    levelEvents[i] = new int[levelSize];
    levelLen[i] = 0;
    }
    activation = new int[levelSize];
}

int gateLevelCkt::retrieveEvent() //fsim
{
    while ((levelLen[currLevel] == 0) && (currLevel < maxlevels))
        currLevel++;

    if (currLevel < maxlevels)
    {
        levelLen[currLevel]--;
        return(levelEvents[currLevel][levelLen[currLevel]]);
    }
    else
        return(-1);
}

//Logic simulate. (no faults inserted)

void gateLevelCkt::goodsim() //fsim
{
    int sucLevel;
    int gateN, predecessor, successor;
    int *predList;
    int i;
    unsigned int val1, val2, tmpVal;
    
	currLevel = 0;
    actLen = actFFLen = 0;
    while (currLevel < maxlevels)
    {
        gateN = retrieveEvent();
        if (gateN != -1)    // if a valid event
        {
            sched[gateN]= 0;
            switch (gtype[gateN])
            {
            case T_and:
                val1 = val2 = ALLONES;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 &= value1[predecessor];
                    val2 &= value2[predecessor];
                }
                break;
            case T_nand:
                val1 = val2 = ALLONES;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 &= value1[predecessor];
                    val2 &= value2[predecessor];
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_or:
                val1 = val2 = 0;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 |= value1[predecessor];
                    val2 |= value2[predecessor];
                }
                break;
            case T_nor:
                val1 = val2 = 0;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 |= value1[predecessor];
                    val2 |= value2[predecessor];
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_not:
                predecessor = faninlist[gateN][0];
                val1 = ALLONES ^ value2[predecessor];
                val2 = ALLONES ^ value1[predecessor];
                break;
            case T_buf:
                predecessor = faninlist[gateN][0];
                val1 = value1[predecessor];
                val2 = value2[predecessor];
                break;
            case T_dff:
                predecessor = faninlist[gateN][0];
                val1 = value1[predecessor];
                val2 = value2[predecessor];
                actFFList[actFFLen] = gateN;
                actFFLen++;
                break;
            case T_xor:
                predList = faninlist[gateN];
                val1 = value1[predList[0]];
                val2 = value2[predList[0]];

                for(i=1; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    tmpVal = ALLONES^(((ALLONES^value1[predecessor]) &
                                       (ALLONES^val1)) | (value2[predecessor]&val2));
                    val2 = ((ALLONES^value1[predecessor]) & val2) |
                           (value2[predecessor] & (ALLONES^val1));
                    val1 = tmpVal;
                }
                break;
            case T_xnor:
                predList = faninlist[gateN];
                val1 = value1[predList[0]];
                val2 = value2[predList[0]];

                for(i=1; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    tmpVal = ALLONES^(((ALLONES^value1[predecessor]) &
                                       (ALLONES^val1)) | (value2[predecessor]&val2));
                    val2 = ((ALLONES^value1[predecessor]) & val2) |
                           (value2[predecessor]& (ALLONES^val1));
                    val1 = tmpVal;
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_output:
                predecessor = faninlist[gateN][0];
                val1 = value1[predecessor];
                val2 = value2[predecessor];
                break;
            case T_input:
            case T_tie0:
            case T_tie1:
            case T_tieX:
            case T_tieZ:
                val1 = value1[gateN];
                val2 = value2[gateN];
                break;
            default:
                fprintf(stderr, "illegal gate type1 %d %d\n", gateN, gtype[gateN]);
                exit(-1);
            }   // switch

            // if gate value changed
            if ((val1 != value1[gateN]) || (val2 != value2[gateN]))
            {
                value1[gateN] = val1;
                value2[gateN] = val2;
                faultValue1[gateN]  = val1;
                faultValue2[gateN]  = val2;

                for (i=0; i<fanout[gateN]; i++)
                {
                    successor = fanoutlist[gateN][i];
                    sucLevel = levelNum[successor];

                    if (sched[successor] != 1)
                    {
                        if (sucLevel != 0)
                            insertEvent(sucLevel, successor);
                        else    // same level, wrap around for next time
                        {
                            activation[actLen] = successor;
                            actLen++;
                        }
                        sched[successor] = 1;
                    }
                }   // for (i...)
            }   // if (val1..)
        }   // if (gateN...)
    }   // while (currLevel...)

}

//	Global fault simulate (faults have been inserted.)
void gateLevelCkt::faultsim()
{
    int sucLevel;
    int gateN, predecessor, successor;
    int *predList;
    int i;
    unsigned int val1, val2, tmpVal;

    currLevel = 1;
    actLen = 0;
    while (currLevel < maxlevels)
    {
        gateN = retrieveEvent();
        if (gateN != -1)    // if a valid event
        {
            sched[gateN]= 0;
            switch (gtype[gateN])
            {
            case T_and:
                val1 = val2 = ALLONES;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 &= faultValue1[predecessor];
                    val2 &= faultValue2[predecessor];
                }
                break;
            case T_nand:
                val1 = val2 = ALLONES;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 &= faultValue1[predecessor];
                    val2 &= faultValue2[predecessor];
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_or:
                val1 = val2 = 0;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 |= faultValue1[predecessor];
                    val2 |= faultValue2[predecessor];
                }
                break;
            case T_nor:
                val1 = val2 = 0;
                predList = faninlist[gateN];
                for (i=0; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    val1 |= faultValue1[predecessor];
                    val2 |= faultValue2[predecessor];
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_not:
                predecessor = faninlist[gateN][0];
                val1 = ALLONES ^ faultValue2[predecessor];
                val2 = ALLONES ^ faultValue1[predecessor];
                break;
            case T_buf:
                predecessor = faninlist[gateN][0];
                val1 = faultValue1[predecessor];
                val2 = faultValue2[predecessor];
                break;
            case T_dff:
                predecessor = faninlist[gateN][0];
                val1 = faultValue1[predecessor];
                val2 = faultValue2[predecessor];
                actFFList[actFFLen] = gateN;
                actFFLen++;
                break;
            case T_xor:
                predList = faninlist[gateN];
                val1 = faultValue1[predList[0]];
                val2 = faultValue2[predList[0]];

                for(i=1; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    tmpVal = ALLONES^(((ALLONES^faultValue1[predecessor]) &
                                       (ALLONES^val1)) | (faultValue2[predecessor]&val2));
                    val2 = ((ALLONES^faultValue1[predecessor]) & val2) |
                           (faultValue2[predecessor] & (ALLONES^val1));
                    val1 = tmpVal;
                }
                break;
            case T_xnor:
                predList = faninlist[gateN];
                val1 = faultValue1[predList[0]];
                val2 = faultValue2[predList[0]];

                for(i=1; i<fanin[gateN]; i++)
                {
                    predecessor = predList[i];
                    tmpVal = ALLONES^(((ALLONES^faultValue1[predecessor]) &
                                       (ALLONES^val1)) | (faultValue2[predecessor]&val2));
                    val2 = ((ALLONES^faultValue1[predecessor]) & val2) |
                           (faultValue2[predecessor]& (ALLONES^val1));
                    val1 = tmpVal;
                }
                tmpVal = val1;
                val1 = ALLONES ^ val2;
                val2 = ALLONES ^ tmpVal;
                break;
            case T_output:
                predecessor = faninlist[gateN][0];
                val1 = faultValue1[predecessor];
                val2 = faultValue2[predecessor];
                break;
            case T_input:
            case T_tie0:
            case T_tie1:
            case T_tieX:
            case T_tieZ:
                val1 = faultValue1[gateN];
                val2 = faultValue2[gateN];
                break;
            default:
                fprintf(stderr, "illegal gate type1 %d %d\n", gateN, gtype[gateN]);
                exit(-1);
            }   // switch
			
            faultValue1[gateN] = val1; //assigning val1 to faultyvalue1
            faultValue2[gateN] = val2; //assigning val2 to faultyvalue2

            for (i=0; i<fanout[gateN]; i++)
            {
                successor = fanoutlist[gateN][i]; 
                sucLevel = levelNum[successor];
            
                if (sched[successor] == 0)
                {
                    if (sucLevel != 0)
                        insertEvent(sucLevel, successor);
                    else  
                    {
                        activation[actLen] = successor;
                        actLen++;
                    }
                    sched[successor] = 1;
                }
            }   // for (i...)

        }   // if (gateN...)
    }   // while (currLevel...)
}
//PODEM Functions

//1.PODEM Function
void gateLevelCkt::podemFunction()
{
	//cout << endl << "PODEM started" << endl;
	cout << endl << "Total number of gates, faulty Included : "<< numgates << endl;
	
	cout << endl << "Original gate type" << endl;
	for (int i =0; i<numberOfFaults; i++)
	{
		cout << originalGateType[i] << " ";
	}
	
	cout << endl << "Faulty Gate numbers : " << endl;
	for (int i =0; i<numberOfFaults; i++)
	{
		cout << faultyGate[i] << " ";
	}	
	
	cout << endl << endl << "Stuck at Faults loaded are : " << endl;
	for (int i =0; i<numberOfFaults; i++)
	{
		cout << stuckAtFault[i] << " ";
	}
	cout << endl << "Total number of faults inserted : " << numberOfFaults << endl;
	
//Gate Substitution Algorithm
for (faultCounter=0; faultCounter<numberOfFaults; faultCounter++)
{
	setDontCare();
	//All the input values are set to Dont Cares 

    int x = gtype[faultyGate[faultCounter]];
    //cout << "Faulty Gate type : " << x << endl;
    
    int previousGateType=gtype[faultyGate[faultCounter]];
    //cout << "previousGateType = " << previousGateType << endl;
    
    //replabacktraceFlagg the faulty gate type by gateReplacerType
    gateReplacer(faultyGate[faultCounter],gateReplacerType[faultCounter]);
    
    int y = gtype[faultyGate[faultCounter]]; 
   //cout<<"After replabacktraceFlagg, new Gate type : "<< y << endl;
    
    if(stuckAtFault[faultCounter]==0)
    {
		//if s-a-0, then faultValue 1 and 2 equals zero
        faultValue1[faultyGate[faultCounter]] =   faultValue2[faultyGate[faultCounter]] = 0;
    }
    else // set to ALLONES
    { 
        faultValue1[faultyGate[faultCounter]] =   faultValue2[faultyGate[faultCounter]] = ALLONES;
    }
    
    
    if((recursivePodem(faultyGate[faultCounter],stuckAtFault[faultCounter])) == true)
    //Getting the vector which has detected the fault   
    {
    cout << endl << "Fault detected at Gate Number : " << faultyGate[faultCounter] << endl;
    cout << "Fault : s-a-" << stuckAtFault[faultCounter] << endl;
    cout << "Faulty Gate type : " << previousGateType << endl;
    cout << "Faulty Gate replaced by gate type : " << gateReplacerType[faultCounter] << endl;     
    cout << "The vector which detects the fault : ";
    
        for (int j = 1; j <= numInputs; j++)
                    {
                        if (value1[j] && value2[j]){
							//If the PI is 1
                            cout << "1";
                        }
                        else if ((value1[j] == 0) && (value2[j] == 0)){
							//If the PI is 0
                            cout << "0";
                        }
                        else{
							cout << "X";
                        }
                        }
                    cout << endl;
                    faultsDetected++;
    }
    else
    {
		//If the fault is not detected
		cout << endl << "Fault is not detected by the PODEM" << endl;
		cout << "Gate Number : " << faultyGate[faultCounter] << endl;
		cout << "Fault : s-a-" << stuckAtFault[faultCounter] << endl;
	}
	
	//ReplabacktraceFlagg the Faulty Gate type by previous Gate Type
    gateReplacer(faultyGate[faultCounter],previousGateType);
}

cout << "\nTotal number of faults = " << numberOfFaults << endl;
cout << "Total number of faults detected are = " << faultsDetected << endl;
float fcoverage = 100*faultsDetected/numberOfFaults;
cout << "\nFault Coverage for this circuit = " << fcoverage << "%" << endl;

}

//2. Recursive PODEM, PODEM algorithm for finding the inputs needed to detect the fault
bool gateLevelCkt::recursivePodem(int fgate, bool fvalue)
{
	bool pathAvailable=false;
    bool backtraceFlag;

    dfront=0;
    getdFrontier(fgate);
    result=false;
    
    int successorGate;

    for(int i=0;i<numOutputs;i++)
    {
		//If Fault effect is observed at PO
		//If result = Success then return Sucess
        if((value1[outputs[i]] && value2[outputs[i]] && !faultValue1[outputs[i]] && !faultValue2[outputs[i]])
            ||(!value1[outputs[i]] && !value2[outputs[i]] && faultValue1[outputs[i]] && faultValue2[outputs[i]]))
        {
            result=true;
            return true;
        }
    }
    //Check if the path is available
    for(int i=0;dFrontier[i]>0;i++)
    { 
        if(xPathCheck(dFrontier[i]))
        {
            pathAvailable=true;
            break;
        }
    }
    //If the path is available, call getObjective and backTrace functions
    if(pathAvailable)
    {
        getObjective(fgate,fvalue);
        backtraceFlag=backTrace(myObjective, objValue);
        if( backtraceFlag == 0 )                                               
            {
                value1[backTraceGate] = value2[backTraceGate] = 0;
                faultValue1[backTraceGate] = faultValue2[backTraceGate] = 0;
            }
        else
            {
				value1[backTraceGate] = value2[backTraceGate] = ALLONES;
                faultValue1[backTraceGate] = faultValue2[backTraceGate] = ALLONES;
            }

        for(int i=0;i<fanout[backTraceGate];i++)
            {
				successorGate = fanoutlist[backTraceGate][i];
					if (sched[successorGate] == 0)
						{
							insertEvent(levelNum[successorGate], successorGate);
							sched[successorGate] = 1;
						}   
            }
            
            goodsim(); 
            flag=faultExcited(fgate); //Check if the fault is excited
		if(fvalue==0)
            {
				//Assigning 1 to both faultValue1 and faultValue2
                faultValue1[fgate] = faultValue2[fgate] = 0;
            }
        else
            {
                faultValue1[fgate] = faultValue2[fgate] = 1;
            }

        for(int i=0;i<fanout[fgate];i++)
            {
             successorGate = fanoutlist[fgate][i];
            if (sched[successorGate] == 0)
                {
                    insertEvent(levelNum[successorGate], successorGate);
                    sched[successorGate] = 1;
                }   
            }
            faultsim();
            
         
            if(recursivePodem(fgate,fvalue) && flag)
            {
                return true;
            }
            
            backtraceFlag=!backtraceFlag;
            if( backtraceFlag == 0 )                                                
            {
                value1[backTraceGate] = value2[backTraceGate] = 0;
                faultValue1[backTraceGate] = faultValue2[backTraceGate] = 0;
            }
        else
            {
                value1[backTraceGate] = value2[backTraceGate] = ALLONES;
                faultValue1[backTraceGate] = faultValue2[backTraceGate] = ALLONES;
            }

        for(int i=0;i<fanout[backTraceGate];i++)
            {
             successorGate = fanoutlist[backTraceGate][i];
            if (sched[successorGate] == 0)
                {
                    insertEvent(levelNum[successorGate], successorGate);
                    sched[successorGate] = 1;
                }   
            }
            goodsim();

            if(fvalue==0)
            {
                faultValue1[fgate] = faultValue2[fgate] = 0;
            }
			else
            {
                faultValue1[fgate] = faultValue2[fgate] = 1;
            }

        for(int i=0;i<fanout[fgate];i++)
            {
             successorGate = fanoutlist[fgate][i];
            if (sched[successorGate] == 0)
                {
                    insertEvent(levelNum[successorGate], successorGate);
                    sched[successorGate] = 1;
                }   
            }
            faultsim();

            if(recursivePodem(fgate,fvalue) && flag)
            {
                return true;
            }

            value1[backTraceGate] = 0;
            value2[backTraceGate] = ALLONES;

            for(int i=0;i<fanout[backTraceGate];i++)
            {
             successorGate = fanoutlist[backTraceGate][i];
            if (sched[successorGate] == 0)
                {
                    insertEvent(levelNum[successorGate], successorGate);
                    sched[successorGate] = 1;
                }   
            }
            goodsim();

            if(fvalue==0)
            {
                faultValue1[fgate] = faultValue2[fgate] = 0;
            }
            else
            {
                faultValue1[fgate] = faultValue2[fgate] = 1;
            }

            for(int i=0;i<fanout[fgate];i++)
            {
             successorGate = fanoutlist[fgate][i];
            if (sched[successorGate] == 0)
                {
                    insertEvent(levelNum[successorGate], successorGate);
                    sched[successorGate] = 1;
                }   
            }
            faultsim();
            return false;
    }
    else
    {
		//return Failure
        //cout << "No x-path, Fault cannot be detected" << endl;
        return false;
    }
}

//3.getObjective, getObjective function for exciting and propogating the fault
void gateLevelCkt::getObjective(int gate, bool value)
{
    objValue=1;
    
    //If fault is not excited, then return (gate, v;)
    if((value1[gate] && !value2[gate])|| (value2[gate]&& !value1[gate]))
    {
        myObjective=gate;
        objValue=!value;
        return;
    }
    
    //d = gate in dFrontier, g = an input whose value is x, v = non-controlling value of d
    //return gate and that noncontrolling value
    int nextGate=dFrontier[0];
    for(int i=0;i<fanin[nextGate];i++)
    {
        if((value1[faninlist[nextGate][i]] && !value2[faninlist[nextGate][i]]) || (!value1[faninlist[nextGate][i]] && value2[faninlist[nextGate][i]]))
        {
            myObjective=faninlist[nextGate][i];
        }
    }
        if(gtype[myObjective]==6 || gtype[myObjective]==7)
            objValue=1;
        else if(gtype[myObjective]==8 || gtype[myObjective]==9)
            objValue=0;
        return;
    
}

//4. backTrace(), backTrace to reach the PI
bool gateLevelCkt::backTrace(int gate, bool value)
{
	//i = g, num_inversion = 0
    int num_inversion=0;
    
    //while i!= primary input
    while(gtype[gate]!=1)   
    {
        //cout << endl << "Backtraced to gate" << gate << endl;
        //if i is inverted gate type then num_inversion++
        if(gtype[gate]==7 || gtype[gate]==9) // Not considering NOT gate
            num_inversion++;
            
            
         for(int i=0;i<fanin[gate];i++)
        {
            if((value1[faninlist[gate][i]] && !value2[faninlist[gate][i]]) || (value2[faninlist[gate][i]] && !value1[faninlist[gate][i]]))
            {
                gate=faninlist[gate][i];
                break;
            }
        }
    }
    
    //if num_inversion = ODD then return v = v'
    if(num_inversion%2!=0)
    {   
		//cout << endl << "Odd" << endl;
        value=!value;
    }
    backTraceGate=gate;
    return value;
}

//5.xPathCheck(), check if the x-path exists to PO
bool gateLevelCkt::xPathCheck(int gatenumber)
{
	//if reached to PO, pathFound = true
    if(gtype[gatenumber]==2)
    {
		pathFound=true;
		return pathFound;
    }
    
    for(int i=0;i<fanout[gatenumber];i++)
    {
		//Check if xPathexists for the fanoutlist, return true if path is found
        if((value1[fanoutlist[gatenumber][i]] && !value2[fanoutlist[gatenumber][i]]) || (!value1[fanoutlist[gatenumber][i]] && value2[fanoutlist[gatenumber][i]])
            ||(faultValue1[fanoutlist[gatenumber][i]] && !faultValue2[fanoutlist[gatenumber][i]]) || (!faultValue1[fanoutlist[gatenumber][i]] && faultValue2[fanoutlist[gatenumber][i]]))
        {
			//cout << endl << "yes" << endl;
            xPathCheck(fanoutlist[gatenumber][i]);
        }
    }
    return pathFound;
}

//6. setDontCare(), Initial Value before starting the PODEM = Dont Care
void gateLevelCkt::setDontCare()
{
    for (int i = 1; i <= numgates; i++)
    {
        value1[i] = faultValue1[i] = 0;
        value2[i] = faultValue2[i] = ALLONES;
    }
}

//7. faultExcited(), Function to ensure that the fault is excited
bool gateLevelCkt::faultExcited(int faultygate)
{
	//AND-NAND, OR-NOR
	//6-98, 7-89, 8-67, 9-76
 
    if   ((originalGateType[faultCounter]==6 &&gateReplacerType[faultCounter]==9) || (originalGateType[faultCounter]==6 &&gateReplacerType[faultCounter]==8)
        || (originalGateType[faultCounter]==7 &&gateReplacerType[faultCounter]==8) || (originalGateType[faultCounter]==7 &&gateReplacerType[faultCounter]==9)
        || (originalGateType[faultCounter]==8 &&gateReplacerType[faultCounter]==6) || (originalGateType[faultCounter]==8 &&gateReplacerType[faultCounter]==7)
        || (originalGateType[faultCounter]==9 &&gateReplacerType[faultCounter]==7) || (originalGateType[faultCounter]==9 &&gateReplacerType[faultCounter]==6))
	{
		return true;
	}
	//check the values of 1's and 0's	
    int ones=0,zeros=0;
    for(int i=0;i<fanin[faultygate];i++)
    {
        if(value1[faninlist[faultygate][i]] && value2[faninlist[faultygate][i]])
            ones++;
        if(!value1[faninlist[faultygate][i]] && !value2[faninlist[faultygate][i]])
            zeros++;
    }
    if(ones>0 && zeros>0)
    {
        return true;
	}
    else{
        return false;
	}
}

//Faulty Gate Insertion, Faults are loaded into the gate level circuits
void gateLevelCkt ::faultSetup()
{
    faultyGate=new int[numgates]; //gate of current fault
    gateReplacerType=new int[numgates]; //type of replaced gate
    originalGateType=new int[numgates]; //type of gate before replabacktraceFlagg
    stuckAtFault=new bool[numgates]; //boolean substitution fault array
    
    int j=0;
    for(int i=0;i<numgates;i++) { //i = 1,2,3,4,5,6 
        if (gtype[i]==6 || gtype[i]==7 || gtype[i]==8 || gtype[i]==9 || gtype[i]==10 || gtype[i]==11)
        //AND, NAND, OR, NOR, Buffer and NOT 
        //gtype is char, not int
        //not able to debug c5315 when type 10 and 11 is considered
        {   
			//All the gates before replabacktraceFlagg are stored in originalGateType
			originalGateType[j]=gtype[i]; 
            originalGateType[j+1]=gtype[i]; //originalGateType[1] = gtype[1]

            switch(gtype[i])
        {
            case 6:
            //AND gate for s-a-0 will be replaced by NOR gate
            //AND gate for s-a-1 will be replaced by OR gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=9;
            gateReplacerType[j+1]=8;
            j=j+2;//j = 2, next set of Gates
            break;
            
            case 7:
            //NAND gate for s-a-0 will be replaced by OR gate
            //NAND gate for s-a-1 will be replaced by NOR gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=8;
            gateReplacerType[j+1]=9;
            j=j+2;//j = 2, next set of Gates
            break;
            
            case 8:
            //OR gate for s-a-0 will be replaced by AND gate
            //OR gate for s-a-1 will be replaced by NAND gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=6;
            gateReplacerType[j+1]=7;
            j=j+2;//j = 2, next set of Gates
            break;
            
            case 9:
            //NOR gate for s-a-0 will be replaced by NAND gate
            //NOR gate for s-a-1 will be replaced by AND gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=7;
            gateReplacerType[j+1]=6;
            j=j+2;//j = 2, next set of Gates
            break;
            
            /*case 10:
            //NOT gate for s-a-0/s-a-1 will be replaced by Buffer gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=11;
            gateReplacerType[j+1]=11;
            j=j+2;//j = 2, next set of Gates
            break;
            
            case 11:
            //Buffer gate for s-a-0/s-a-1 will be replaced by NOT gate
            stuckAtFault[j]=0;
            stuckAtFault[j+1]=1;
            faultyGate[j]=i;
            faultyGate[j+1]=i;
            gateReplacerType[j]=10;
            gateReplacerType[j+1]=10;
            j=j+2;//j = 2, next set of Gates
            break;*/
            }
          }
        }
    numberOfFaults=j;
}

//Getting the D-frontier for the given gate  
void gateLevelCkt::getdFrontier( int gate)
{
	//cout << "value1[gate] = " << value1[gate] << endl; 
	//If d-frontier exists, that gate is stored in the dFrontier Array
    if(((value1[gate] == 0 && value2[gate]) || (faultValue1[gate] == 0 && faultValue2[gate]) || (value1[gate] && value2[gate]==0) || (faultValue1[gate] && faultValue2[gate]==0) ))
    {
        dFrontier[dfront]=gate;
        dfront++;
        //cout << endl << "dFrontier[] " << dfront << endl;
    }
    else if((value1[gate] && value2[gate] && (faultValue1[gate] == 0) && (faultValue2[gate] == 0)) || (value1[gate] == 0 && value2[gate] == 0 && (faultValue1[gate]) && (faultValue2[gate])))
    {
        for(int i = 0; i <fanout[gate]; i++)
        {
            getdFrontier(fanoutlist[gate][i]);
        }
    }
    dFrontier[dfront]=0;
    //Check dFrontier[]
}

//Faulty Gate Substitution, Inserting the fault by replacing the gate
void gateLevelCkt::gateReplacer(int gateno, int gateReplacerType)
{
    gtype[gateno]=gateReplacerType;
}

int main(int argc, char *argv[])
{
	char cktName[10]; //name of the circuit
	if (argc != 2)
	{
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
        }
    strcpy(cktName, argv[1]);//cktName = c17
	circuit = new gateLevelCkt(cktName);
	circuit->faultSetup();
	circuit->podemFunction();
}



