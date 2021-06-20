#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <queue>
#include <sstream>
#include <windows.h>
using namespace std;
ifstream fin;
ofstream fout;
char err[50];


HANDLE hCon;
void SetColor(int C)
{
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hCon, C);

}
struct ProcessControlBlock{
    int jobId;
	int TTL;   // total time limit
	int TLL;   // total line limit
	int TTC;   // total time count
	int LLC;   // line limit count
    int npc;    //no of program card
    int ndc;    //no of data cards
    int noc;    //no of output cards
    int pc;     //first program card
    int dc;     //first data card
    int oc=-1;     //first output card on drum

};
class Execute
{
public:
    void begin();
    Execute(){
        for (int i = 0; i < 100; i++)
            for (int j = 0; j < 4; j++)
                Buffer[i][j] = '-';

        for (int i = 0; i < 500; i++)
            for (int j = 0; j < 4; j++)
                Drum[i][j] = '-';
        for(int i=0;i<10;i++) ebq.push(i);
        for(int i=0;i<50;i++) free_drum.push(i);
    }

private:
    char M[300][4], Buffer[100][4], Drum[500][4], R[4], IR[4], C;     // declaration of registers
    int IC, mem_pointer = 0 ;
    int VA=0, PTR = 0, RA = 0, PTE = 0;
    int  SI = 0, TI = 0, PI = 0, EM = 0;   // declaration of interrupt
    int free_blocks[30] = {0};             // to keep track of available m/m blocks
    int no_flag=0, OSC=0, TSC=0;
    char end_loop;
    int IOI, CHF[3], CHT[3];

    queue<struct ProcessControlBlock> lq, rq, tq, ioq;
    queue<int> ebq, ifbq, ofbq, free_drum;

    void initialize();
    void start_execution();
    void Execute_User_Program();
    void MOS();
    void Terminate(int);
    int allocate();
    int addressMap(int VA);
    void terminateFun(char err[50]);
    void Simulation();
    void print_mem(const char mem[][4], int n);
    void IR1();
    void CH1(int b);
    void IR2();
    void CH2(int b);
    void IR3(string task);
    void CH3(string task);
};

void Execute::begin(){
    initialize();
    IOI=1;
    Simulation();
}

void Execute::initialize(){

    std::cout << "I am in initialize function \n";

    int i, j;
    // Reset Memory    M <- 0
    for (i = 0; i < 300; i++)
        for (j = 0; j < 4; j++)
            M[i][j] = '-';

    // reset registers
    for (i = 0; i < 4; i++)
    {
        IR[i] = '0';
        R[i] = '0';
    }
    for (i = 0; i < 3; i++)
    {
        CHF[i] = 0;
        CHT[i] = 0;
    }

    // reset free blocks
    for (i=0; i<30; i++) free_blocks[i] = 0;

    C = 'F';
    mem_pointer = 0;
    EM = 0;
    SI = 0;
    TI=0;
    PI=0;
    IOI=1;
}

void Execute::Simulation(){




    while(1){
    std::cout<<"In simulation function"<<endl;
    TSC=0;
    if(CHF[0]==0 &&(IOI==1|| IOI==3 ||IOI==5 || IOI==7)) IR1();
    if(CHF[2]==0 &&(IOI==4|| IOI==5 ||IOI==6 || IOI==7) ) IR3("IS");
    if(CHF[1]==0 &&(IOI==2|| IOI==3 ||IOI==6 || IOI==7) ) IR2();
    else break;
    }
}

void Execute :: print_mem(const char mem[][4], int n)
{
    int i,j,k;
    for (i = 0;i < n;i+=10)                    //print memory content on screen
    {   if (mem[i][0]!='-')
        {
            for (k=i; k<i+10; k++){
            std::cout << k << " ";
            for (j = 0; j < 4; j++)
                std::cout << mem[k][j];
            std::cout << endl;
            }
        }
        else std::cout << i << " " << mem[i][0] << mem[i][1]<<mem[i][2] << mem[i][3]<<endl;
    }
}

int Execute::allocate() {
    std::cout << "I am in Allocate function \n";
    int n = rand()%30;
    if(free_blocks[n] == 0)    // checking if memory block is free
    {
        free_blocks[n] = -1;
        return n;
    }
    else
    {
        return allocate();
    }
}

int Execute::addressMap(int VA){
    string temp;
    std::cout << "I am in Address Map function for VA="<<VA<<endl;
    if (VA>99 || VA<0)
    {
        std::cout << "PI=2\n";
        PI=2;      // operand error
        return -1;
    }
    PTE = PTR + VA/10;
    std::cout << "PTE = " <<PTE << " & M[PTE] = " << M[PTE][0] << endl;

    if (M[PTE][0]=='-')  // page fault error
    {
        std::cout << "PI=3\n";
        PI=3;
        MOS();
        RA+= + (VA %10);
        if(IR[0]=='G' || IR[0]=='S')return RA;
        else return -1;
    }
    for (int i=0; i<4; i++)
        temp += M[PTE][i];
    int offset = stoi(temp);
    RA = (offset * 10) + (VA %10);
    std::cout << "RA = " << RA<<endl;
    return RA;
}

void Execute::start_execution(){
    std::cout << "I am in start execution function \n";
    IC = 0;
    VA = 0;
    // print_mem(M, 300);
    Execute_User_Program();
}

void Execute::Execute_User_Program()
{
    std::cout << "I am in execute user program function \n";
    end_loop='c';
    string opcode;
    int i, j, operand;

    while (1)
    {
        TSC=0;
        RA = addressMap(IC);
        if (PI!=0)
        {
            end_loop = 'F';
            break;
        }
        for (i = 0; i < 4; i++)                         // loading instruction into IR from memory
            IR[i] = M[RA][i];
        IC++;                                  // increment instruction counter

        if(IR[0] == 'H')                     // Halt and TTL err
        {
            rq.front().TTC++;
            if (rq.front().TTC > rq.front().TTL)
            {
                TI=2;
                MOS();
                break;
            }
            SI = 3;
            MOS();
            end_loop='c';
            break;
        }

        opcode = "";                            // calculate opcode & operand
        opcode += IR[0];
        opcode += IR[1];
        if(opcode[0]=='G' || opcode[0]=='P') IR[3]='0';

        // Checking for operand error
        string S;
        try
        {
            S= IR[2];
            int ir2 = stoi(S);
            S=IR[3];
            int ir3 = stoi(S);
            operand = (ir2)*10 + ir3;
        }
        catch(exception a)
        {
            no_flag=-1;
        }

        if(no_flag == -1)
        {
            no_flag = 0;
            PI=2;
            MOS();
            end_loop='c';
            break;
        }

        //Checking for opcode error
        std::cout<<"Checking Opcode error "<<endl;
        if( !((opcode.compare("GD"))== 0 || (opcode.compare("PD")) == 0 ||(opcode.compare("SR")) == 0 ||(opcode.compare("LR")) == 0 ||(opcode.compare("BT")) == 0 ||(opcode.compare("CR")) == 0 ||(opcode.compare("H")) == 0)   )
        {

            PI = 1;
            MOS();
            end_loop='c';
            break;
        }
        //Checking Time limit exceed err
        if(IR[0] != 'H')
        {
            rq.front().TTC++;
            TSC++;
        }
        if (rq.front().TTC > rq.front().TTL)
        {
            TI=2;
            MOS();
            end_loop='c';
            break;
        }

        std::cout << "opcode = " << opcode << endl;
        std::cout << "operand = " << operand << endl;
        std::cout << "ttc = " << rq.front().TTC << endl;

        RA = addressMap(operand);
        std::cout << "RA = " << RA << endl;

        if (RA==-1)   //Valid Page Fault
        {
            break;
        }

        if (opcode == "LR")                      // load the register R by contents of memory
        {
            std::cout <<  "R = ";
            for (j = 0; j < 4; j++)
            {
                R[j] = M[RA][j];
                std::cout << R[j];
            }
            std::cout << endl;
        }

        else if (opcode == "SR")                 // store the register R in memory
        {
            rq.front().TTC++;
            TSC++;
            for (j = 0; j < 4; j++)
            {
                M[RA][j] = R[j];
            }
            print_mem(M,300);
        }

        else if (opcode == "CR")                 // compare reg R with content at memory location
        {
            for (j=0; j<4; j++)
            {
                if (R[j] == M[RA][j]) C = 'T';
                else{
                    C= 'F';
                    break;
                }
            }
        }

        else if (opcode == "BT")                 // Branch if C is true
        {
            if (C == 'T')
            {
                IC = operand;
            }

        }

        else if (opcode == "GD")                 // get data
        {
            rq.front().TTC++;
            TSC++;
            SI = 1;
            MOS();
            if(EM==1) break;
            print_mem(M,300);
        }

        else if (opcode == "PD")                 // put data
        {
            SI = 2;
            MOS();
            if(EM==2) break;
        }

    OSC+=TSC;
    TSC=0;
    cout<<"OSC="<<OSC<<endl;
    }
}

void Execute::MOS()  // master mode
{
    int i, j;
    string temp;
    std::cout << "I am in MOS function \n";
    std::cout<<"TI= "<<TI<<" PI= "<<PI<<" SI= "<<SI<<endl;

    if (TI==0)
    {
        if (PI!=0)
        {
            if (PI==1)
            {
                //opcode error
                PI=0;
                EM = 4;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
            else if(PI==2)
            {
                PI=0;
                EM = 5;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");

            }
            else if (PI==3 && (IR[0]=='G' || IR[0]=='S'))
            {
                std::cout << "Handling valid page fault\n";
                mem_pointer = allocate();             // Get frame for program card
                std::cout << "Frame Got : " << mem_pointer<< endl;
                j = PTR;

                while(M[j][0]!='-')                   // updating page table
                {
                    j++;
                }
                for (int k=0;k<4;k++)
                    M[j][k]='0';
                temp = to_string(mem_pointer);
                i = temp.length();
                for (int k=0; k<i; k++)
                    M[j][3-k] = temp[i-k-1];

                PI = 0;
                RA=mem_pointer*10;
                // print_mem(M,300);
            }
            else if (PI==3 && IR[0]!='G')
            {
                PI=0;
                EM = 6;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
        }
        else
        {
            // handling SI
            if (SI==1)
            {
                SI=0;
                ioq.push(rq.front());
                rq.pop();
                IR3("GD");
            }
            else if (SI==2)
            {
                SI=0;
                ioq.push(rq.front());
                rq.pop();
                IR3("PD");
            }
            else if (SI==3)
            {
                SI=0;
                tq.push(rq.front());
                rq.pop();
                EM = 0;
                IR3("OS");
            }
        }
    }
    else if (TI==2)
    {
        // handling SI
        if (PI==0)
        {
            if (SI==1)
            {
                SI=0;
                EM = 3;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
            else if (SI==2)
            {
                SI=0;
                EM = 3;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
            else if (SI==3)
            {
                SI=0;
                EM = 0;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
            else if (SI==0)
            {
                EM = 3;
                tq.push(rq.front());
                rq.pop();
                IR3("OS");
            }
        }
        else
        {
            PI=0;
            EM = 3;
            tq.push(rq.front());
            rq.pop();
            IR3("OS");
        }
    }
}

void Execute::Terminate(int EM)
{
    std::cout << "I am in Terminate function \n";
    SetColor(12);    // Seting the Colour to RED
    switch(EM)
    {
        case 0: SetColor(15);   // Reseting the colour to white
                std::cout<<"NO ERROR"<<endl;
                strcpy(err,"No error");
                break;

        case 1:
                std::cout<<"\nError (1) :Out Of Data Error"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                      // Reseting the colour to white
                strcpy(err,"Out of Data Error");
                terminateFun(err);
                break;
        case 2:
                std::cout<<"\nError (2) :Line limit Exceeded"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                             // Reseting the colour to white
                strcpy(err,"Line limit exceeded");
                terminateFun(err);
                break;
        case 3:
                std::cout<<"\nError(3) :Time Limit Exceeded"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                             // Reseting the colour to white
                strcpy(err,"Time limit exceeded");
                terminateFun(err);
                break;
        case 4:
                std::cout<<"\nError(4) :Operation Code Error"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                               // Reseting the colour to white
                strcpy(err,"Operation code error");
                terminateFun(err);
                break;
        case 5:
                std::cout<<"\nError(5) :Operand Error"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                               // Reseting the colour to white
                strcpy(err,"Operand error");
                terminateFun(err);
                break;
        case 6:
                std::cout<<"\nError (6) :Invalid Page Fault"<<endl;
                std::cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                                      // Reseting the colour to white
                strcpy(err,"Invalid page fault");
                terminateFun(err);
                break;
    }
}

void Execute::terminateFun(char err[50]){
    string buffer, temp;
    std::cout << "I am in Terminatefun function \n";
    stringstream ss;
    ss<<"IR:";
    for(int i=0;i<4;i++)
        ss<<IR[i];

    ss<<" Job_ID="<<tq.front().jobId;
    ss<<" IC="<<IC;
    ss<<" TI="<<TI<<" PI="<<PI<<" SI="<<SI;
    ss<<" TTC="<<tq.front().TTC<<" TTL="<<tq.front().TTL<<" LLC="<<tq.front().LLC<<" TLL="<<tq.front().TLL;

    string msg = ss.str();
    int b = ebq.front();
    ebq.pop();
    int mem_loc = 0;
    for (int i = 0; i < 10; i++){
        for (int j = 0; j < 4; j++){
            if(err[mem_loc]!='\0')Buffer[b*10 + i][j]=err[mem_loc++];
        }
    }
    ofbq.push(b);
    b = ebq.front();
    mem_loc=0;
    ebq.pop();
    for (int i = b*10; i < b*10 + 10; i++){
        for (int j = 0; j < 4; j++){
            if(mem_loc<37)Buffer[i][j]=msg[mem_loc++];
        }
    }
    ofbq.push(b);
    b = ebq.front();
    ebq.pop();
    for (int i = b*10; i < b*10 + 10; i++){
        for (int j = 0; j < 4; j++){
            if(mem_loc<msg.length())Buffer[i][j]=msg[mem_loc++];
        }
    }
    ofbq.push(b);
    std::cout<<err<<' '<<msg<<endl;
}

void Execute::IR1(){
    cout<<"I am in IR1"<<endl;
    if(!ebq.empty()){
        int b=ebq.front();
        CH1(b);
    }
    IOI+=4;
}

void Execute::CH1(int b){
    cout<<"I am in CH1"<<endl;
    IOI-=1;
    CHT[0]=0;
    CHF[0]=1;
    string card;
    char F;
    int offset=0;
    getline(fin, card);
    if(card=="") return;
    while(card!="" && fin){
        if(card[0]=='$'){
            string header = card.substr(0, 4);    // checks first 4 characters of line
            if (header == "$AMJ")
            {

                SetColor(13);
                std::cout<<"\n\n\n---------------------- New Program ----------------------\n\n";
                SetColor(15);

                struct ProcessControlBlock PCB;
                PCB.jobId = stoi(card.substr(4,4));       //Initializing PCB block;
                PCB.TTL = stoi(card.substr(8,4));
                PCB.TLL=stoi(card.substr(12,4));
                PCB.TTC=0;
                PCB.LLC=0;
                PCB.ndc=0;
                PCB.npc=0;
                PCB.pc=b;
                lq.push(PCB);

                std::cout<<"Program ID = "<<PCB.jobId<<endl;
                F='P';
            }else if(header == "$DTA"){
                F='D';
                lq.front().dc = ebq.front();
            }else if(header == "$END"){
                std::cout<<"Loaded Program ID ="<<lq.front().jobId<<" in Buffer storage"<<endl;
                break;
            }
        }else{
            if(F=='P'){
                lq.front().npc+=1;
                b=ebq.front();
                ebq.pop();
                mem_pointer =b*10;
                int i = 0;
                int j = 0;

                while (i < card.length() && i < 40 )            // loading program into frame
                {
                    if (card[i] == 'H')              // for Halt instruction !=D is for operand error
                    {
                        Buffer[mem_pointer][0] = 'H';
                        mem_pointer++;
                        i++;
                    }
                    else                               // for other instructions
                    {
                        string temp = card.substr(i, 4);     // reading one instruction at a time

                        for (j=0; j<4; j++)
                        {
                            Buffer[mem_pointer][j] = temp[j];
                        }
                        mem_pointer++;
                        i+=4;
                    }
                }
                ifbq.push(b);
            }else if(F=='D'){
                b = ebq.front();
                ebq.pop();
                lq.front().ndc+=1;
                mem_pointer =b*10;
                int i = 0;
                int j = 0;
                while (i < card.length() && i < 40 ){
                    string temp = card.substr(i, 4);
                    for (j=0; j<4; j++){
                        if(j>=temp.length())Buffer[mem_pointer][j]='-';
                        else Buffer[mem_pointer][j] = temp[j];
                    }
                    mem_pointer++;
                    i+=4;
                }
                ifbq.push(b);
            }
        }
        getline(fin, card);
    }
    CHF[0]=0;
    IOI+=1;
    CHT[0]=5;
    OSC+=CHT[0];
    cout<<"OSC="<<OSC<<endl;
    // print_mem(Buffer, 100);

}

void Execute::IR3(string task){
    std::cout<<"I am in IR3"<<endl;
    if(!ifbq.empty() && !free_drum.empty()){
        CH3("IS");
    }if(!lq.empty()){
        CH3("LD");
    }if(!rq.empty()){
        start_execution();
    }

    if(task=="GD") {
        CH3(task);
    }
    if(task=="PD") {
        CH3(task);
    }
    if(task=="OS"){
        CH3(task);
    }
}

void Execute::CH3(string task){
    std::cout<<"In ch3 for "<<task<<endl;
    IOI-=4;
    TSC=0;
    CHT[2]=0;
    CHF[2]=1;
    if(task=="IS"){
        lq.front().pc=free_drum.front();
        int nc=0;
        while(!ifbq.empty() && !free_drum.empty()){
            if(nc==lq.front().npc) lq.front().dc=free_drum.front();
            int d = free_drum.front();
            int b = ifbq.front();
            free_drum.pop();
            ifbq.pop();
            mem_pointer = d*10;
            for(int i=0;i<10;i++){
                for(int j=0;j<4;j++){
                    Drum[mem_pointer+i][j]=Buffer[b*10 + i][j];
                    Buffer[b*10 + i][j]='-';
                }
            }
            nc++;
            ebq.push(b);
            cout<<"Program loaded in Drum."<<endl;
        }
    }else if(task=="LD"){
        PTR = allocate() * 10;
        std::cout << "PTR = " << PTR << endl;
        for(int pcc=0; pcc<lq.front().npc; pcc++){      //program card count

            mem_pointer = allocate();             // Get frame for program card
            std::cout << "Frame Got : " << mem_pointer<< endl;
            int j = PTR;

            while(M[j][0]!='-')                   // updating page table
                j++;
            for (int k=0;k<4;k++) M[j][k]='0';

            string temp = to_string(mem_pointer);
            int i = temp.length();

            for (int k=0; k<i; k++)
                M[j][3-k] = temp[i-k-1];
            mem_pointer *=10;
            int d=(lq.front().pc + pcc)%50;
            for(int i=0;i<10;i++){
                for(int j=0;j<4;j++){
                    M[mem_pointer + i][j] = Drum[d*10+i][j];
                    Drum[d*10+i][j]='-';
                }
            }
        free_drum.push(d);
        }
        rq.push(lq.front());
        lq.pop();
        cout<<"Program loaded in Main memory. ready for execution"<<endl;
    }else if(task=="GD"){
        int mem_loc, i, j;
        string buffer;
        std::cout << "RA="<<RA<<endl;
        mem_loc = RA;    // calculation of memory address
        if (ioq.front().ndc==0)
        {
            EM = 1;
            tq.push(ioq.front());
            ioq.pop();
            IR3("OS");
            return;
        }
        int d = ioq.front().dc;
        for(int i=0;i<10;i++){
            for(int j=0;j<4;j++){
                M[RA+i][j]=Drum[d*10 + i][j];
                Drum[d*10 + i][j]='-';
            }
        }
        ioq.front().ndc-=1;
        if(ioq.front().ndc!=0) ioq.front().dc=(ioq.front().dc+1)%50;
        free_drum.push(d);
        rq.push(ioq.front());
        ioq.pop();
        cout<<"Data loaded in Main memory"<<endl;
    }else if(task=="PD"){
        ioq.front().LLC+= 1;
        if(ioq.front().LLC > ioq.front().TLL)
        {
            EM = 2;
            tq.push(ioq.front());
            ioq.pop();
            IR3("OS");  // Terminate 2
            return;
        }
        int mem_loc;
        mem_loc = RA;    // calculation of memory address
        int d = free_drum.front();
        free_drum.pop();
        for(int i=0;i<10;i++){
            for(int j=0;j<4;j++){
                Drum[d*10 + i][j]=M[RA+i][j];
            }
        }
        if(ioq.front().oc==-1){
            ioq.front().oc=d;
            ioq.front().noc=0;
        }
        ioq.front().noc+=1;
        rq.push(ioq.front());
        ioq.pop();
        cout<<"Data loaded from main memory to Drum"<<endl;
    }else if(task=="OS"){
        int d = tq.front().oc;
        if(d!=-1){
            tq.front().oc=ebq.front();
            for(int occ=0;occ<tq.front().noc;occ++){
                int b = ebq.front();
                ebq.pop();
                int mem_loc = ((d+occ)%50)*10;
                for (int i = 0; i < 10; i++){
                    for (int j = 0; j < 4; j++){
                        Buffer[(b)*10+i][j]=Drum[mem_loc+i][j];
                        Drum[mem_loc + i][j]='-';
                    }
                }
                ofbq.push(b);
                free_drum.push((d+occ)%50);
            }
        }
        Terminate(EM);
        ofbq.push(ebq.front());
        ebq.pop();
        ofbq.push(ebq.front());
        ebq.pop();
        tq.pop();
        IOI+=2;
        std::cout<<"---------------------------"<<endl;
    }
    CHF[2]=0;
    IOI+=4;
    CHT[2]=2;
    OSC+=CHT[2];
    cout<<"OSC="<<OSC<<endl;
}

void Execute::IR2(){
    cout<<"I am in IR2"<<endl;
    if(!ofbq.empty()){
        int b=ofbq.front();
        CH2(b);
    }
}

void Execute::CH2(int b){
    cout<<"I am in CH2"<<endl;
    IOI-=2;
    TSC=0;
    CHT[1]=0;
    CHF[1]=1;
    while(!ofbq.empty()){
        b=ofbq.front();
        for(int i=0;i<10;i++){
            for(int j=0;j<4;j++){
                if(Buffer[b*10 +i][j]=='-') fout<<' ';
                else fout<<Buffer[b*10 +i][j];
                Buffer[b*10+i][j]='-';
            }
        }
        fout<<endl;
        ebq.push(ofbq.front());
        ofbq.pop();
    }
    IOI+=2;
    CHT[1]=5;
    CHF[1]=0;
    OSC+=CHT[1];
    cout<<"OSC="<<OSC<<endl;
    initialize();
}

int main(){
    Execute e;
    fin.open("input3.txt");           // opening file to read from
    fout.open("output3.txt");         // opening file to write to
    e.begin();
    return 0;
}
