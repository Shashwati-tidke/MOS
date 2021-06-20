#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <windows.h>
using namespace std;


//Removing error of operand when first lette is H

// SetColor Changes the text color of Output Text of Console
HANDLE hCon;
void SetColor(int C)
{
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hCon, C);

}

ifstream fin;
ofstream fout;


char err[50];
struct ProcessControlBlock
{
    int jobId;

	int TTL;   // total time limit
	int TLL;   // total line limit

	int TTC;   // total time count
	int LLC;   // line limit count
};


class Execute
{
public:
    void load();

private:
    struct ProcessControlBlock PCB;
    char M[300][4], R[4], IR[4], C;     // declaration of registers
    int IC, mem_pointer = 0 ;
    int VA=0, PTR = 0, RA = 0, PTE = 0;
    int  SI = 0, TI = 0, PI = 0, EM = 0;   // declaration of interrupt
    int free_blocks[30] = {0};             // to keep track of available m/m blocks
    int no_flag=0;
    char end_loop;


    void initialize();
    void start_execution();
    void Execute_User_Program();
    void MOS();
    void Read();
    void Write();
    void Terminate(int);
    int allocate();
    int addressMap(int VA);
    void terminateFun(char err[50]);
    void Simulation();
    void print_mem();

};

void Execute :: print_mem()
{
    int i,j,k;
    for (i = 0;i < 300;i+=10)                    //print memory content on screen
    {   if (M[i][0]!='-')
        {
            for (k=i; k<i+10; k++){
            cout << k << " ";
            for (j = 0; j < 4; j++)
                cout << M[k][j];
            cout << endl;
            }
        }
        else cout << i << " " << M[i][0] << M[i][1]<<M[i][2] << M[i][3]<<endl;
    }
}

int Execute::addressMap(int VA)
{
    string temp;
    cout << "I am in Address Map function \n";
    if (VA>99 || VA<0)
    {
        cout << "PI=2\n";
        PI =2;     // operand error
        return -1;
    }
    PTE = PTR + VA/10;
    cout << "PTE = " <<PTE << " & M[PTE] = " << M[PTE][0] << endl;

    if (M[PTE][0]=='-')  // page fault error
    {
        cout << "PI=3\n";
        PI=3;
        return -1;
    }
    for (int i=0; i<4; i++)
        temp += M[PTE][i];
    int offset = stoi(temp);
    RA = (offset * 10) + (VA %10);
    cout << "RA = " << RA<<endl;
    return RA;
}

int Execute::allocate()  //Random number generator
{
    cout << "I am in Allocate function \n";
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

void Execute::MOS()  // master mode
{
    int i, j;
    string temp;
    cout << "I am in MOS function \n";
    cout<<"TI= "<<TI<<" PI= "<<PI<<endl;

    if (TI==0)
    {
        if (PI!=0)
        {
            if (PI==1)
            {
                //opcode error
                PI=0;
                EM = 4;
                Terminate(EM);
            }
            else if(PI==2)
            {
                //operand error not thought off
                PI=0;
                EM = 5;
                Terminate(EM);

            }
            else if (PI==3 && (IR[0]=='G' || IR[0]=='S'))
            {
                //cout<<"Valid GD increment"<<endl;
                PCB.TTC+=2;
                cout << "Handling valid page fault\n";
                mem_pointer = allocate();             // Get frame for program card
                cout << "Frame Got : " << mem_pointer<< endl;
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
                IC--;
                PI = 0;
                Execute_User_Program();
            }
            else if (PI==3 && IR[0]!='G')
            {
                PI=0;
                EM = 6;
                Terminate(EM);
            }
        }
        else
        {
            // handling SI
            if (SI==1)
            {
                SI=0;
                Read();
            }
            else if (SI==2)
            {
                SI=0;
                Write();
            }
            else if (SI==3)
            {
                SI=0;
                EM = 0;
                Terminate(EM);
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
                Terminate(EM);
            }
            else if (SI==2)
            {
                Write();
                SI=0;
                EM = 3;
                Terminate(EM);
            }
            else if (SI==3)
            {
                SI=0;
                EM = 0;
                Terminate(EM);
            }
            else if (SI==0)
            {
                EM = 3;
                Terminate(EM);
            }
        }
        // Handling PI
        //if (PI>0 && PI<4)
        else
        {
            PI=0;
            EM = 3;
            Terminate(EM);
        }
    }
}



void Execute::Read()                          // Read Data card
{
    cout << "I am in Read function \n";
    int mem_loc, i, j;
    string buffer;
    IR[3] = '0';
    cout << "RA="<<RA<<endl;
    mem_loc = RA;    // calculation of memory address
    getline(fin, buffer);                      // reading line from input.txt
    if (buffer[0]=='$')
    {
        EM = 1;
        Terminate(EM);
    }
    i = 0;
    j = 0;
    while (j < buffer.length())                   // fill buffer content into memory location
    {
        M[mem_loc][i] = buffer[j];
        cout << mem_loc << " " << i << " : ";
        cout << M[mem_loc][i] << " ";
        cout << endl;
        i++;
        j++;
        if (i == 4)                              // after 4 bytes increase memory location
        {
            i = 0;
            mem_loc++;
        }
    }



    Execute_User_Program();
}

void Execute::Write()                        // function to write to output file
{
    cout << "I am in Write function \n";
    PCB.LLC+= 1;
    if(PCB.LLC > PCB.TLL)
    {
        EM = 2;
        Terminate(2);  // Terminate 2
    }
    int mem_loc, i, j;
    IR[3] = '0';
    mem_loc = RA;    // calculation of memory address
    for (i = mem_loc; i < mem_loc + 10; i++)         // write a word block to output file
    {
        for (j = 0; j < 4; j++)
        {
            cout << i << " " << j << " : ";
            if (M[i][j] == '-') { fout << " "; continue; }
            fout << M[i][j];
            cout << M[i][j] << " ";
        }
        cout << endl;
    }
    fout << "\n";

    PCB.TTC-=1;

    Execute_User_Program();
}

void Execute::Terminate(int EM)
{
    cout << "I am in Terminate function \n";
    SetColor(12);    // Seting the Colour to RED
    switch(EM)
    {
        case 0: SetColor(15);   // Reseting the colour to white
                cout<<"NO ERROR"<<endl;
                strcpy(err,"No error");
                break;

        case 1:
                cout<<"\nError (1) :Out Of Data Error"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                      // Reseting the colour to white
                strcpy(err,"Out of Data Error");
                terminateFun(err);
                break;
        case 2:
                cout<<"\nError (2) :Line limit Exceeded"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                             // Reseting the colour to white
                strcpy(err,"Line limit exceeded");
                terminateFun(err);
                break;
        case 3:
                cout<<"\nError(3) :Time Limit Exceeded"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                             // Reseting the colour to white
                strcpy(err,"Time limit exceeded");
                terminateFun(err);
                break;
        case 4:
                cout<<"\nError(4) :Operation Code Error"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                               // Reseting the colour to white
                strcpy(err,"Operation code error");
                terminateFun(err);
                break;
        case 5:
                cout<<"\nError(5) :Operand Error"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                               // Reseting the colour to white
                strcpy(err,"Operand error");
                terminateFun(err);
                break;
        case 6:
                cout<<"\nError (6) :Invalid Page Fault"<<endl;
                cout<<"Program Terminated Abnormally"<<endl;
                SetColor(15);                                                      // Reseting the colour to white
                strcpy(err,"Invalid page fault");
                terminateFun(err);
                break;
    }
}


void Execute::terminateFun(char err[50])
{
    string buffer, temp;
    cout << "I am in Terminatefun function \n";
    fout<<err<<endl;
    fout<<"IR: ";
    for(int i=0;i<4;i++)
        fout<<IR[i];
    fout<<endl;

    fout<<"Job_ID= "<<PCB.jobId<<endl;
    fout<<"IC= "<<IC<<endl;
    fout<<"TI= "<<TI<<"  PI="<<PI<<"  SI="<<SI<<endl;
    fout<<"TTC= "<<PCB.TTC<<" TTL= "<<PCB.TTL<<" LLC= "<<PCB.LLC<<" TLL= "<<PCB.TLL<<endl;
    fout<<endl;
    fout<<endl;

    if(EM != 1)                                      //For avoiding line skip when EM=1
    getline(fin, buffer);

    temp = buffer.substr(0,4);
    while(temp!="$END" && temp!="")
          {
              cout << "buffer = " << buffer<<endl;
              getline(fin, buffer);
              temp =buffer.substr(0,4);
          }
    load();
}



//*******************************************************************************************************
void Execute::load()
{
    cout << "I am in Load function\n";
    int i, j, buffer_length;
    string header, temp, buffer;

    mem_pointer = 0;

    getline(fin, buffer);                       // reading line from input.txt
    //cout << "buffer = " << buffer << endl;
loop:while (buffer!="" && fin)                                  // While eof
    {
        if (buffer[0] == '$')                  // check first char of line
        {                                      // Control card

            header = buffer.substr(0, 4);    // checks first 4 characters of line
            if (header == "$AMJ")
            {

                SetColor(13);
                cout<<"\n\n\n---------------------- New Program ----------------------\n\n";
                SetColor(15);

                PCB.jobId = stoi(buffer.substr(4,4));       //Initializing PCB block;
                PCB.TTL = stoi(buffer.substr(8,4));
                PCB.TLL=stoi(buffer.substr(12,4));
                PCB.TTC=0;
                PCB.LLC=0;

                cout<<"Program ID = "<<PCB.jobId<<endl;

                initialize();  // Calling initialize fucntion

                PTR = allocate() * 10;
                cout << "PTR = " << PTR << endl;

            }
            else if (header == "$DTA")
            {
                start_execution();
            }
            else if (header == "$END")
            {
                fout << "\n\n";
                // reading next line from input file
                getline(fin, buffer);

                if (buffer=="")
                {
                    fin.close();
                    fout.close();
                    exit(0);
                }
                else
                    goto loop;
            }
        }
        else                                           // Program Card
        {
            mem_pointer = allocate();             // Get frame for program card
            cout << "Frame Got : " << mem_pointer<< endl;
            j = PTR;

            while(M[j][0]!='-')                   // updating page table
            {
                j++;
            }
            for (int k=0;k<4;k++) M[j][k]='0';
            temp = to_string(mem_pointer);
            i = temp.length();
            for (int k=0; k<i; k++)
                M[j][3-k] = temp[i-k-1];
            mem_pointer *=10;
            i = 0;
            j = 0;
            buffer_length = buffer.length();



            while (i < buffer_length && i < 40 )            // loading program into frame
            {
                if (buffer[i] == 'H' && buffer[i+1] != 'D')              // for Halt instruction !=D is for operand error
                {
                    M[mem_pointer][0] = 'H';
                    mem_pointer++;
                    i++;
                }
                else                               // for other instructions
                {
                    temp = buffer.substr(i, 4);     // reading one instruction at a time

                    for (j=0; j<4; j++)
                    {
                        M[mem_pointer][j] = temp[j];
                    }
                    mem_pointer++;
                    i+=4;
                }
            }
            print_mem();
        }
        getline(fin, buffer);                         // read next line from input file
        cout << "buffer = " << buffer << endl;

    }
    fin.close();
    fout.close();
    exit(0);

}

//--------------------------------------------------------------------------------------------------------

void Execute::start_execution()
{
    cout << "I am in start execution function \n";
    IC = 0;
    VA = 0;
    Execute_User_Program();
}


void Execute::Execute_User_Program()
{
    cout << "I am in execute user program function \n";
    end_loop='0';
    string opcode;
    int i, j, operand;

    while (1)
    {

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
            PCB.TTC++;
            if (PCB.TTC > PCB.TTL)
            {
                TI=2;
                MOS();
            }
            SI = 3;
            MOS();
            break;
        }

            opcode = "";                            // calculate opcode & operand
            opcode += IR[0];
            opcode += IR[1];

            // Checking for operand error
            string S;
            try                     //for IRr
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
            }

            //Checking for opcode error
            cout<<"Checking Opcode error "<<endl;
            if( !((opcode.compare("GD"))== 0 || (opcode.compare("PD")) == 0 ||(opcode.compare("SR")) == 0 ||(opcode.compare("LR")) == 0 ||(opcode.compare("BT")) == 0 ||(opcode.compare("CR")) == 0 ||(opcode.compare("H")) == 0)   )
            {

                PI = 1;
                MOS();
            }
            //Checking Time limit exceed err
            cout<<"Checking Time error "<<endl;
            /*if((opcode.compare("GD"))== 0 || (opcode.compare("SR")) == 0)
            {

                cout<<"GD increment"<<endl;
                PCB.TTC+=1;
            }
            else
                */
            if(IR[0] != 'H' )
            {
                PCB.TTC++;
            }
            if (PCB.TTC > PCB.TTL)
            {
                TI=2;
                MOS();
            }

            cout << "opcode = " << opcode << endl;
            cout << "operand = " << operand << endl;

            RA = addressMap(operand);

            if (PI!=0)   //Valid Page Fault
            {
                end_loop = 'E';
                break;
            }

            if (opcode == "LR")                      // load the register R by contents of memory
            {
                cout <<  "R = ";
                for (j = 0; j < 4; j++)
                {
                    R[j] = M[RA][j];
                    cout << R[j];
                }
                cout << endl;
            }

            else if (opcode == "SR")                 // store the register R in memory
            {
                for (j = 0; j < 4; j++)
                {
                    M[RA][j] = R[j];
                }
                print_mem();
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
                SI = 1;
                MOS();                               // go to master mode

            }

            else if (opcode == "PD")                 // put data
            {

                SI = 2;
                MOS();

            }
        }

    print_mem();
    Simulation();
    if (end_loop=='F' || end_loop=='E') MOS();

}

void Execute::Simulation()
{
    SetColor(2);
    cout << "I am in simulation function \n";
    cout << "TTC = " << PCB.TTC << " & TTL = "<<PCB.TTL<<endl;
    cout << "SI = "<<SI << " PI = " << PI << " TI = "<<TI<<endl;
    SetColor(15);

    if (SI!=0 || PI!=0 || TI!=0)
        MOS();
    load();
}

void Execute::initialize()                  // reset memory and registers
{
    cout << "I am in initialize function \n";

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

    // reset free blocks
    for (i=0; i<30; i++) free_blocks[i] = 0;

    C = 'F';
    mem_pointer = 0;
    EM = 0;
    SI = 0;
    TI=0;
    PI=0;

}

int main()
{
    Execute e;
    fin.open("input.txt");           // opening file to read from
    fout.open("output.txt");         // opening file to write to
    e.load();                        // call to load function

    return 0;
}
