//
// Created by fred on 11/05/18.
//

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <bitset>
#include <cstring>
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


void Emulator::reset()
{
    //Reset registers, and set stack pointer to top (it grows downwards)
    reg = {0};
    reg.SP = sizeof(memory) - 1;

    //Setup register lookup tables
    reg_table_r = {&reg.general.B, &reg.general.C, &reg.general.D, &reg.general.E, &reg.general.H, &reg.general.L, (uint8_t*)&reg.general.HL, &reg.general.A};
    reg_table_rp = {&reg.general.BC, &reg.general.DE, &reg.general.HL, &reg.SP};
    reg_table_rp2 = {&reg.general.BC, &reg.general.DE, &reg.general.HL, &reg.general.AF};

    reg_table_r_names = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    reg_table_rp_names = {"BC", "DE", "HL", "SP"};
    reg_table_rp2_names = {"BC", "DE", "HL", "AF"};
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
        Prefix prefix = to_prefix(memory[reg.PC]);
        if(prefix != Prefix::None)
            ++reg.PC;

        uint8_t x, y, z, p, q;
        x = (memory[reg.PC] >> 6) & 0x3;
        y = (memory[reg.PC] >> 3) & 0x7;
        z = memory[reg.PC] & 0x7;
        p = (y >> 1) & 0x3;
        q = (y >> 2) & 0x2;

        switch(prefix)
        {
            case Prefix::None:
            {
                switch(x)
                {
                    case 0: // X =
                    {
                        switch(z)
                        {
                            case 0: // z = 0
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
                                    case 3:
                                        break;
                                    case 4: case 5: case 6: case 7:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            case 1: // z = 1
                            {
                                switch(q)
                                {
                                    case 0: // LD rp[p], nn
                                    {
                                        *reg_table_rp[p] = memory[reg.PC + 1] | (memory[reg.PC + 2] << 8);
                                        reg.PC += 2;

                                        log_stream << "LD " << reg_table_rp_names[p].c_str() << ", " << *reg_table_rp[p] << std::endl;
                                        break;
                                    }
                                    case 1:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            }
                            case 2:
                                break;
                            case 3: // Z = 3
                            {
                                switch(q)
                                {
                                    case 0: // INC rp[p]
                                    {
                                        ++(*reg_table_rp[p]);


                                        log_stream << "INC " << reg_table_rp_names[p] << std::endl;
                                        break;
                                    }
                                    case 1: // DEC rp[p]
                                    {
                                        --(*reg_table_rp[p]);
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
                                reg.general.F.PV = *reg_table_r[y] == 0x7F; // If overflow
                                reg.general.F.H = (((*reg_table_r[y] & 0xF) + 1) & 0x10) == 0x10; // If half carry

                                ++(*reg_table_r[y]); // Do the increment

                                reg.general.F.S = *reg_table_r[y] >> 7; // If result is negative, set S
                                reg.general.F.Z = *reg_table_r[y] == 0; // If result is 0
                                reg.general.F.N = 0;

                                log_stream << "INC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 5: // DEC r[y]
                            {
                                reg.general.F.PV = *reg_table_r[y] == 0x80; // If underflow
                                reg.general.F.H = ((int8_t)(((*reg_table_r[y] & 0xF) - 1)) < 0);

                                --(*reg_table_r[y]); // Do the decrement

                                reg.general.F.S = *reg_table_r[y] >> 7; // If result is negative, set S
                                reg.general.F.Z = *reg_table_r[y] == 0; // If result is 0
                                reg.general.F.N = 1;

                                log_stream << "DEC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 6: // LD r[y], n
                            {
                                *reg_table_r[y] = memory[++reg.PC];

                                log_stream << "LD " << reg_table_r_names[y].c_str() << ", " << (uint16_t)*reg_table_r[y] << std::endl;
                                break;
                            }
                            case 7:
                                break;
                            default:
                                abort();
                        }
                    }
                    case 1: // X = 1
                        break;
                    case 2: // X = 2, alu[y] r[z]
                    {
                        switch(y)
                        {
                            case 0: // ADD A, r[z]
                            {
                                uint8_t result = reg.general.A + *reg_table_r[z];

                                reg.general.F.S = result >> 7; // If result is negative, set S
                                reg.general.F.Z = result == 0; // If result is 0
                                reg.general.F.PV = ((reg.general.A >> 7) == (*reg_table_r[z] >> 7) && (*reg_table_r[z] >> 7) != reg.general.F.S); // If overflow
                                reg.general.F.H = (((reg.general.A & 0xF) + *reg_table_r[z]) & 0x10) == 0x10; // If carry from bit 3
                                reg.general.F.C = (((uint16_t)reg.general.A) + ((uint16_t)*reg_table_r[z])) > 0xFF; // If carry from bit 7
                                reg.general.F.N = 0;

                                reg.general.A = result;
                                log_stream << "ADD A, " << reg_table_r_names[z] << std::endl;
                                break;
                            }
                            case 1: // ADC A, r[z]
                                break;
                            case 2: // SUB r[z]

                            {
                                uint8_t result = reg.general.A - *reg_table_r[z];

                                reg.general.F.S = result >> 7; // If result is negative
                                reg.general.F.Z = result == 0; // If result is 0
                                reg.general.F.H = ((reg.general.A & 0xF) < (*reg_table_r[z] & 0xF)); // If borrow from bit 4
                                reg.general.F.PV = ((reg.general.A >> 7) == (*reg_table_r[z] >> 7) && (*reg_table_r[z] >> 7) != reg.general.F.S); // If overflow
                                reg.general.F.N = 1;
                                reg.general.F.C = std::abs(reg.general.A) + std::abs(*reg_table_r[z]) > 0xFF; // If borrow

                                reg.general.A = result;
                                log_stream << "SUB " << reg_table_r_names[z] << std::endl;
                                break;
                            }
                            case 3: // SBC A, r[z]
                                break;
                            case 4: // AND r[z]
                            {
                                uint8_t result = reg.general.A & *reg_table_r[z];

                                reg.general.F.S = result >> 7; // If result is negative, set S
                                reg.general.F.Z = result == 0; // If result is 0
                                reg.general.F.H = 1;
                                reg.general.F.PV = parity(result); // If parity
                                reg.general.F.N = 0;
                                reg.general.F.C = 0;

                                reg.general.A = result;
                                log_stream << "AND " << reg_table_r_names[z] << std::endl;
                                break;
                            }
                            case 5: // XOR r[z]
                            {
                                uint8_t result = reg.general.A ^ *reg_table_r[z];

                                reg.general.F.S = result >> 7; // If result is negative, set S
                                reg.general.F.Z = result == 0; // If result is 0
                                reg.general.F.H = 0;
                                reg.general.F.PV = parity(result); // If parity
                                reg.general.F.N = 0;
                                reg.general.F.C = 0;

                                reg.general.A = result;
                                log_stream << "XOR " << reg_table_r_names[z] << std::endl;
                                break;
                            }
                            case 6: // OR r[z]
                            {
                                uint8_t result = reg.general.A | *reg_table_r[z];

                                reg.general.F.S = result >> 7; // If result is negative, set S
                                reg.general.F.Z = result == 0; // If result is 0
                                reg.general.F.H = 0;
                                reg.general.F.PV = parity(result); // If parity
                                reg.general.F.N = 0;
                                reg.general.F.C = 0;

                                reg.general.A = result;
                                log_stream << "OR " << reg_table_r_names[z] << std::endl;
                                break;
                            }
                            case 7: // CP r[z]
                                break;
                            default:
                                abort();
                        }
                        break;
                    }
                    case 3: // X = 3
                        switch(z)
                        {
                            case 0:
                                break;
                            case 1: // Z = 1
                            {
                                switch(q)
                                {
                                    case 0: // POP rp2[p]
                                    {
                                        *reg_table_rp2[p] = pop();
                                        log_stream << "POP " << reg_table_rp2_names[p] << std::endl;
                                        break;
                                    }
                                    case 1: // Q = 1
                                    {
                                        switch(p)
                                        {
                                            case 0:
                                                break;
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
                            case 4:
                                break;
                            case 5:
                                switch(q)
                                {
                                    case 0: // PUSH rp2[p]
                                    {
                                        push(*reg_table_rp2[p]);

                                        log_stream << "PUSH " << reg_table_rp2_names[p] << std::endl;
                                        break;
                                    }
                                    case 1:
                                        break;
                                    default:
                                        abort();
                                }
                                break;
                            case 6:
                                break;
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
            default:
                abort();
        }
    }
}
