#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

ifstream fin;
ofstream fout;



class Execute
{
public:
    void load();

private:
    char M[100][4], R[4], IR[4], C;     // declaration of registers
    int IC, mem_pointer = 0, SI = 0;

    void initialize();
    void start_execution();
    void Execute_User_Program();
    void MOS();
    void Read();
    void Write();
    void Terminate();
};

void Execute::MOS()  // master mode
{
    switch (SI)
    {
    case 1:
        Read();
        break;
    case 2:
        Write();
        break;
    case 3:
        Terminate();
        break;
    }
}

void Execute::Read()                          // Read Data card
{
    int mem_loc, i, j;
    string buffer;
    IR[3] = '0';
    mem_loc = (IR[2] - '0') * 10 + (IR[3] - '0');    // calculation of memory address
    getline(fin, buffer);                      // reading line from input.txt

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
    int mem_loc, i, j;
    IR[3] = '0';
    mem_loc = (IR[2] - '0') * 10 + (IR[3] - '0');    // calculation of memory address
    cout << "mem_loc = " << mem_loc << endl;
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
    Execute_User_Program();
}

void Execute::Terminate()
{
    fout << "\n\n";                           // two programs are separated by 2 blank lines
    load();
}

void Execute::load()
{
    int i, j;
    string header, temp, buffer;

    mem_pointer = 0;

    getline(fin, buffer);                       // reading line from input.txt

    while (fin)                                  // While eof
    {
        if (buffer[0] == '$')                  // check first char of line
        {                                      // Control card

            header = buffer.substr(0, 4);    // checks first 4 characters of line
            if (header == "$AMJ")
            {
                cout << "\n\n\n\n\n New Program\n";
                initialize();
            }
            else if (header == "$DTA")
            {
                start_execution();
            }
            else if (header == "$END")
            {
                break;
            }
        }
        else                                           // Program Card
        {
            // if (mem_pointer == 100) abort(memory_exceeded)  // not in phase 1
            i = 0;
            j = 0;
            while (i < buffer.length())
            {
                if (buffer[i] == 'H')              // for Halt instruction
                {
                    M[mem_pointer][0] = 'H';
                    mem_pointer++;
                    j = 0;
                }
                else                               // for other instructions
                {
                    temp = buffer.substr(i, 4);     // reading one instruction at a time
                    M[mem_pointer][j] = buffer[i];
                    j++;
                    if (j == 4)                      // after 4 bytes increase memory location
                    {
                        j = 0;
                        mem_pointer++;
                    }
                }
                i++;
            }
            for (i = 0;i < 100;i++)                    //print memory content on screen
            {
                cout << i << " ";
                for (j = 0; j < 4; j++)
                    cout << M[i][j];
                cout << endl;
            }


        }
        getline(fin, buffer);                         // read next line from input file

    }

}

void Execute::start_execution()
{
    IC = 0;
    Execute_User_Program();
}


void Execute::Execute_User_Program()
{
    string opcode;
    int i, j, operand;

    while (IC < mem_pointer)
    {
        for (i = 0; i < 4; i++)                         // loading instruction into IR from memory
            IR[i] = M[IC][i];
        IC++;                                       // increment instruction counter
        if (IR[0] != 'H')
        {
            opcode = "";                            // calculate opcode & operand
            opcode += IR[0];
            opcode += IR[1];
            operand = (IR[2] - '0') * 10 + (IR[3] - '0');

            cout << "opcode = " << opcode << endl;
            cout << "operand : " << operand << endl;

            if (opcode == "LR")                      // load the register R by contents of memory
            {
                for (j = 0; j < 4; j++)
                {
                    R[j] = M[operand][j];
                }
            }

            else if (opcode == "SR")                 // store the register R in memory
            {
                for (j = 0; j < 4; j++)
                {
                    M[operand][j] = R[j];
                }
            }

            else if (opcode == "CR")                 // compare reg R with content at memory location
            {
                for (j=0; j<4; j++)
                {
                    cout << R[j] << " compare " << M[operand][j] << endl;
                    if (R[j] == M[operand][j]) C = 'T';
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
        else                                   // halt instruction
        {
            SI = 3;
            MOS();
        }
    }
}


void Execute::initialize()                  // reset memory and registers
{
    int i, j;
    // Reset Memory    M <- 0
    for (i = 0; i < 100; i++)
        for (j = 0; j < 4; j++)
            M[i][j] = '-';


    // reset registers
    for (i = 0; i < 4; i++)
    {
        IR[i] = '0';
        R[i] = '0';
    }
    C = 'F';
    mem_pointer = 0;
    SI = 0;

}

int main()
{
    Execute e;
    fin.open("input.txt");           // opening file to read from
    fout.open("output.txt");         // opening file to write to
    e.load();                        // call to load function
    fin.close();
    fout.close();
    return 0;
}
