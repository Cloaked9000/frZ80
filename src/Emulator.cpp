//
// Created by fred on 11/05/18.
//

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <bitset>
#include <cstring>
#include <Emulator.h>

#include "Emulator.h"

Emulator::Emulator()
{
    reset();
}

void Emulator::bind_port(uint16_t port_no, Emulator::port_handler_t handler)
{
    ports[port_no] = std::move(handler);
}

Emulator::Prefix Emulator::to_prefix(uint8_t val)
{
    switch(val)
    {
        case 0xCB:
            return CB;
        case 0xDD:
            return DD;
        case 0xED:
            return ED;
        case 0xFD:
            return FD;
        default:
            return None;
    }
    abort();
}


void Emulator::alu_add(uint8_t val)
{
    uint8_t result = reg.general.A + val;

    reg.general.F.S = result >> 7; // If result is negative, set S
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.PV = ((reg.general.A >> 7) == (val >> 7) && (val >> 7) != reg.general.F.S); // If overflow
    reg.general.F.H = (((reg.general.A & 0xF) + val) & 0x10) == 0x10; // If carry from bit 3
    reg.general.F.C = (((uint16_t)reg.general.A) + ((uint16_t)val)) > 0xFF; // If carry from bit 7
    reg.general.F.N = 0;

    reg.general.A = result;
}

void Emulator::alu_adc(uint8_t val)
{
    uint8_t result = reg.general.A + val + reg.general.F.C;

    reg.general.F.S = result >> 7; // If result is negative, set S
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.PV = ((reg.general.A >> 7) == (val >> 7) && (val >> 7) != reg.general.F.S); // If overflow
    reg.general.F.H = (((reg.general.A & 0xF) + val) & 0x10) == 0x10; // If carry from bit 3
    reg.general.F.C = (((uint16_t)reg.general.A) + ((uint16_t)val) + (uint16_t)reg.general.F.C) > 0xFF; // If carry from bit 7
    reg.general.F.N = 0;

    reg.general.A = result;
}

void Emulator::alu_sub(uint8_t val)
{
    uint8_t result = reg.general.A - val;

    reg.general.F.S = result >> 7; // If result is negative
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = ((reg.general.A & 0xF) < (val & 0xF)); // If borrow from bit 4
    reg.general.F.PV = ((reg.general.A >> 7) == (val >> 7) && (val >> 7) != reg.general.F.S); // If overflow
    reg.general.F.N = 1;
    reg.general.F.C = std::abs(reg.general.A) + std::abs(val) > 0xFF; // If borrow

    reg.general.A = result;
}

void Emulator::alu_sbc(uint8_t val)
{
    uint8_t result = reg.general.A - val - reg.general.C;

    reg.general.F.S = result >> 7; // If result is negative
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = (reg.general.A & 0xF) < (val & 0xF) + reg.general.C; // If borrow from bit 4
    reg.general.F.PV = ((reg.general.A >> 7) == (val >> 7) && (val >> 7) != reg.general.F.S); // If overflow
    reg.general.F.N = 1;
    reg.general.F.C = std::abs(reg.general.A) + std::abs(val) + reg.general.C > 0xFF; // If borrow

    reg.general.A = result;
}
void Emulator::alu_and(uint8_t val)
{
    uint8_t result = reg.general.A & val;

    reg.general.F.S = result >> 7; // If result is negative, set S
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = 1;
    reg.general.F.PV = parity(result); // If parity
    reg.general.F.N = 0;
    reg.general.F.C = 0;

    reg.general.A = result;
}

void Emulator::alu_xor(uint8_t val)
{
    uint8_t result = reg.general.A ^ val;

    reg.general.F.S = result >> 7; // If result is negative, set S
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = 0;
    reg.general.F.PV = parity(result); // If parity
    reg.general.F.N = 0;
    reg.general.F.C = 0;

    reg.general.A = result;
}

void Emulator::alu_or(uint8_t val)
{
    uint8_t result = reg.general.A | val;

    reg.general.F.S = result >> 7; // If result is negative, set S
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = 0;
    reg.general.F.PV = parity(result); // If parity
    reg.general.F.N = 0;
    reg.general.F.C = 0;

    reg.general.A = result;
}

void Emulator::alu_cp(uint8_t val)
{
    uint8_t result = reg.general.A - val;

    reg.general.F.S = result >> 7; // If result is negative
    reg.general.F.Z = result == 0; // If result is 0
    reg.general.F.H = ((reg.general.A & 0xF) < (val & 0xF)); // If borrow from bit 4
    reg.general.F.PV = ((reg.general.A >> 7) == (val >> 7) && (val >> 7) != reg.general.F.S); // If overflow
    reg.general.F.N = 1;
    reg.general.F.C = std::abs(reg.general.A) + std::abs(val) > 0xFF; // If borrow
}

void Emulator::bli_ldi()
{

}

void Emulator::bli_cpi()
{

}

void Emulator::bli_ini()
{

}

void Emulator::bli_outi()
{

}

void Emulator::bli_ldd()
{

}

void Emulator::bli_cpd()
{

}

void Emulator::bli_ind()
{

}

void Emulator::bli_outd()
{

}

void Emulator::bli_ldir() //note: This wont behave realistically if the instruction overwrites itself
{
    do
    {
        memory[reg.general.DE++] = memory[reg.general.HL++];
    } while(--reg.general.BC != 0);
    reg.general.F.H = 0;
    reg.general.F.N = 0;
    reg.general.F.PV = reg.general.BC - 1 != 0;
}

void Emulator::bli_cpir()
{

}

void Emulator::bli_inir()
{

}

void Emulator::bli_otir()
{

}

void Emulator::bli_lddr()
{

}

void Emulator::bli_cpdr()
{

}

void Emulator::bli_indr()
{

}

void Emulator::bli_otdr()
{

}


void Emulator::reset()
{
    //Reset registers, and set stack pointer to top (it grows downwards)
    reg = {0};
    reg.SP = sizeof(memory) - 1;

    //Setup register lookup tables
    reg_table_r = {&reg.general.B, &reg.general.C, &reg.general.D, &reg.general.E, &reg.general.H, &reg.general.L, (uint8_t*)&reg.general.HL, &reg.general.A};
    reg_table_rp = {&reg.general.BC, &reg.general.DE, &reg.general.HL, &reg.SP};
    reg_table_rp2 = {&reg.general.BC, &reg.general.DE, &reg.general.HL, &reg.general.AF};
    alu_table = {std::bind(&Emulator::alu_add, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_adc, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_sub, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_sbc, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_and, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_xor, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_or, this, std::placeholders::_1),
                 std::bind(&Emulator::alu_cp, this, std::placeholders::_1)};

    bli_table =  std::array<std::array<std::function<void()>, 4>, 4>
                 {{{std::bind(&Emulator::bli_ldi, this), std::bind(&Emulator::bli_cpi, this), std::bind(&Emulator::bli_ini, this), std::bind(&Emulator::bli_outi, this)},
                 {std::bind(&Emulator::bli_ldd, this), std::bind(&Emulator::bli_cpd, this), std::bind(&Emulator::bli_ind, this), std::bind(&Emulator::bli_outd, this)},
                 {std::bind(&Emulator::bli_ldir, this), std::bind(&Emulator::bli_cpir, this), std::bind(&Emulator::bli_inir, this), std::bind(&Emulator::bli_otir, this)},
                 {std::bind(&Emulator::bli_lddr, this), std::bind(&Emulator::bli_cpdr, this), std::bind(&Emulator::bli_indr, this), std::bind(&Emulator::bli_otdr, this)}}};


    reg_table_r_names = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    reg_table_rp_names = {"BC", "DE", "HL", "SP"};
    reg_table_rp2_names = {"BC", "DE", "HL", "AF"};
    cc_table_names = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
    alu_table_names = {"ADD A,", "ADC A,", "SUB", "SBC A,", "AND", "XOR", "OR", "CP"};
    bli_table_names = {{{"LDI", "CPI", "INI", "OUTI"},
                        {"LDD", "CPD", "IND", "OUTD"},
                        {"LDIR", "CPIR", "INIR", "OTIR"},
                        {"LDDR", "CPDR", "INDR", "OTDR"}}};
}

void Emulator::push(uint16_t val)
{
    memory[reg.SP--] = (val >> 8) & 0xFF; // Stop top 8 bits first
    memory[reg.SP--] = val & 0xFF; // Stop bottom 8 bits last
}

uint16_t Emulator::pop()
{
    uint16_t data = (memory[reg.SP] << 8) | memory[reg.SP + 1];
    reg.SP += 2;

    return data;
}

void Emulator::emulate(const std::vector<uint8_t> &data, std::ostream &log_stream)
{
    //Copy the program data into memory first
    memcpy(memory, data.data(), data.size());

    for(reg.PC = 0; reg.PC < data.size(); ++reg.PC)
    {
        //Get instruction prefix
        process_instruction:
        Prefix prefix = to_prefix(memory[reg.PC]);
        if(prefix != Prefix::None)
            ++reg.PC;

        uint8_t x, y, z, p, q;
        x = (memory[reg.PC] >> 6) & 0x3;
        y = (memory[reg.PC] >> 3) & 0x7;
        z = memory[reg.PC] & 0x7;
        p = (y >> 1) & 0x3;
        q = y & 0x1;

        switch(prefix)
        {
            case Prefix::None:
            {
                switch(x)
                {
                    case 0: // X = 0
                    {
                        switch(z)
                        {
                            case 0: // z = 0
                            {
                                switch(y)
                                {
                                    case 0: // NOP
                                    {
                                        break;
                                    }
                                    case 1: // EX AF, AF'
                                    {
                                        std::swap(reg.general.AF, reg.shadow.AF);
                                        log_stream << "EX AF, AF'" << std::endl;
                                        break;
                                    }
                                    case 2:
                                        break;
                                    case 3: // JR d
                                    {
                                        int8_t jmp_offset = data[++reg.PC] + 2;
                                        reg.PC += jmp_offset - 1;

                                        log_stream << "JR " << (int16_t)jmp_offset << std::endl;
                                        goto process_instruction;
                                        break;
                                    }
                                    case 4: case 5: case 6: case 7:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 1: // z = 1
                            {
                                switch(q)
                                {
                                    case 0: // LD rp[p], nn
                                    {
                                        *get_rp_reg(p) = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                        reg.PC += 2;

                                        log_stream << "LD " << reg_table_rp_names[p].c_str() << ", " << *get_rp_reg(p) << std::endl;
                                        break;
                                    }
                                    case 1:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 2: // z = 2
                            {
                                switch(q)
                                {
                                    case 0: // q = 0
                                    {
                                        switch(p)
                                        {
                                            case 0: // LD (BC), A
                                            {
                                                memory[reg.general.BC] = reg.general.A;
                                                log_stream << "LD (BC), A" << std::endl;
                                                break;
                                            }
                                            case 1: // LD (DE), A
                                            {
                                                memory[reg.general.DE] = reg.general.A;
                                                log_stream << "LD (DE), A" << std::endl;
                                                break;
                                            }
                                            case 2: // LD (nn), HL
                                            {
                                                uint16_t addr = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                                reg.PC += 2;

                                                memory[addr] = reg.general.L;
                                                memory[addr + 1] = reg.general.H;

                                                log_stream << "LD (" << addr << "), HL" << std::endl;
                                                break;
                                            }
                                            case 3: // LD (nn), A
                                            {
                                                uint16_t addr = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                                reg.PC += 2;

                                                memory[addr] = reg.general.A;

                                                log_stream << "LD (" << addr << "), A" << std::endl;
                                                break;
                                            }
                                            default:
                                                abort();
                                        }
                                        break;
                                    }
                                    case 1: // Q = 1
                                    {
                                        switch(p)
                                        {
                                            case 0: // LD A, (BC)
                                            {
                                                reg.general.A = memory[reg.general.BC];
                                                log_stream << "LD A, (BC)" << std::endl;
                                                break;
                                            }
                                            case 1: // LD A, (DE)
                                            {
                                                reg.general.A = memory[reg.general.DE];
                                                log_stream << "LD A, (DE)" << std::endl;
                                                break;
                                            }
                                            case 2: // LD HL, (nn)
                                            {
                                                uint16_t addr = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                                reg.PC += 2;

                                                reg.general.L = memory[addr];
                                                reg.general.H = memory[addr + 1];

                                                log_stream << "LD HL, (" << addr << ")" << std::endl;
                                                break;
                                            }
                                            case 3: // LD A, (nn)
                                            {
                                                uint16_t addr = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                                reg.PC += 2;

                                                reg.general.A = memory[addr];

                                                log_stream << "LD A, (" << addr << ")" << std::endl;
                                                break;
                                            }
                                            default:
                                                abort();
                                        }
                                        break;
                                    }
                                    default:
                                        abort();
                                }

                                break;
                            }
                            case 3: // z = 3
                            {
                                switch(q)
                                {
                                    case 0: // INC rp[p]
                                    {
                                        ++(*get_rp_reg(p));


                                        log_stream << "INC " << reg_table_rp_names[p] << std::endl;
                                        break;
                                    }
                                    case 1: // DEC rp[p]
                                    {
                                        --(*get_rp_reg(p));
                                        log_stream << "DEC " << reg_table_rp_names[p] << std::endl;
                                        break;
                                    }
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 4: // INC r[y]
                            {
                                uint8_t *y_reg = get_r_reg(y);

                                reg.general.F.PV = *y_reg == 0x7F; // If overflow
                                reg.general.F.H = (((*y_reg & 0xF) + 1) & 0x10) == 0x10; // If half carry

                                ++(*y_reg); // Do the increment

                                reg.general.F.S = *y_reg >> 7; // If result is negative, set S
                                reg.general.F.Z = *y_reg == 0; // If result is 0
                                reg.general.F.N = 0;

                                log_stream << "INC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 5: // DEC r[y]
                            {
                                uint8_t *y_reg = get_r_reg(y);

                                reg.general.F.PV = *y_reg == 0x80; // If underflow
                                reg.general.F.H = ((int8_t)(((*y_reg & 0xF) - 1)) < 0);

                                --(*y_reg); // Do the decrement

                                reg.general.F.S = *y_reg >> 7; // If result is negative, set S
                                reg.general.F.Z = *y_reg == 0; // If result is 0
                                reg.general.F.N = 1;

                                log_stream << "DEC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 6: // LD r[y], n
                            {
                                uint8_t *y_reg = get_r_reg(y);
                                *y_reg = memory[++reg.PC];

                                log_stream << "LD " << reg_table_r_names[y].c_str() << ", " << (uint16_t)*y_reg << std::endl;
                                break;
                            }
                            case 7:
                                break;
                            default:
                                abort();
                        }
                        break;
                    }
                    case 1: // X = 1
                    {
                        if(z == 6 && y == 6) // HALT
                        {
                            // todo: implement, currently just exits
                            reg.PC = data.size();
                            log_stream << "HALT" << std::endl;
                            break;
                        }
                        else // LD r[y], r[z]
                        {

                            *get_r_reg(y) = *get_r_reg(z);
                            log_stream << "LD " << reg_table_r_names[y] << ", " << reg_table_r_names[z] << std::endl;
                        }
                        break;
                    }
                    case 2: // X = 2, alu[y] r[z]
                    {
                        alu_table[y](*reg_table_r[z]);
                        log_stream << alu_table_names[y] << " " << reg_table_r_names[z] << std::endl;
                        break;
                    }
                    case 3: // X = 3
                        switch(z)
                        {
                            case 0: // Z = 0, RET cc[y]
                            {
                                log_stream << "RET " << cc_table_names[y] << std::endl;
                                if(get_cc_value(y))
                                {
                                    reg.PC = pop();
                                    goto process_instruction;
                                }
                                break;
                            }
                            case 1: // Z = 1
                            {
                                switch(q)
                                {
                                    case 0: // POP rp2[p]
                                    {
                                        *get_rp2_reg(p) = pop();
                                        log_stream << "POP " << reg_table_rp2_names[p] << std::endl;
                                        break;
                                    }
                                    case 1: // Q = 1
                                    {
                                        switch(p)
                                        {
                                            case 0: // RET
                                            {
                                                reg.PC = pop();

                                                log_stream << "RET" <<std::endl;
                                                goto process_instruction;
                                                break;
                                            }
                                            case 1: // EXX
                                            {
                                                std::swap(reg.general.BC, reg.shadow.BC);
                                                std::swap(reg.general.DE, reg.shadow.DE);
                                                std::swap(reg.general.HL, reg.shadow.HL);

                                                log_stream << "EXX" << std::endl;
                                                break;
                                            }
                                            case 2:
                                                break;
                                            case 3:
                                                break;
                                            default:
                                                abort();
                                        }
                                        break;
                                    }
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 2:
                                break;
                            case 3: // Z = 3
                            {
                                switch(y)
                                {
                                    case 0:
                                        break;
                                    case 1:
                                        break;
                                    case 2: // OUT (n), A
                                    {
                                        out(memory[++reg.PC], &reg.general.A);
                                        log_stream << "OUT (" << (uint16_t)memory[reg.PC] << "), A" << std::endl;
                                        break;
                                    }
                                    case 3: // IN A, (n)
                                    {
                                        in(memory[++reg.PC], &reg.general.A);
                                        log_stream << "IN A, (" << (uint16_t)memory[reg.PC] << ")" << std::endl;
                                        break;
                                    }
                                    case 4:
                                        break;
                                    case 5:
                                        break;
                                    case 6:
                                        break;
                                    case 7:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 4: // Z = 4
                            {
                                break;
                            }
                            case 5: // Z = 5
                                switch(q)
                                {
                                    case 0: // PUSH rp2[p]
                                    {
                                        push(*get_rp2_reg(p));

                                        log_stream << "PUSH " << reg_table_rp2_names[p] << std::endl;
                                        break;
                                    }
                                    case 1:
                                    {
                                        switch(p)
                                        {
                                            case 0: // CALL nn
                                            {
                                                uint16_t call_dest = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                                push(reg.PC + 3);

                                                reg.PC = call_dest;

                                                log_stream << "CALL " << call_dest << std::endl;
                                                goto process_instruction;
                                                break;
                                            }
                                            default:
                                                abort();
                                        }
                                        break;
                                    }
                                    default:
                                        abort();
                                }
                                break;
                            case 6: // 	alu[y] n
                            {
                                alu_table[y](memory[++reg.PC]);
                                log_stream << alu_table_names[y] << " " << (uint16_t)memory[reg.PC] << std::endl;
                                break;
                            }
                            case 7:
                                break;
                            default:
                                abort();
                        }
                        break;
                    default:
                        abort();
                }
                break;
            }
            case Prefix::CB:
            {
                break;
            }
            case Prefix::ED:
            {
                switch(x)
                {
                    case 0: // NONI
                    {
                        break;
                    }
                    case 1:
                    {
                        break;
                    }
                    case 2: // x = 2
                    {
                        if(z <= 3 && y >= 4) // BLI[y, z]
                        {
                            bli_table[y - 4][z]();
                            log_stream << bli_table_names[y - 4][z] << std::endl;
                            break;
                        }
                        break; // NONI
                    }
                    case 3: // NONI
                    {
                        break;
                    }
                }
                break;
            }
            default:
                abort();
        }
    }
}