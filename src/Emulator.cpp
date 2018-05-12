//
// Created by fred on 11/05/18.
//

#include <iostream>
#include <array>
#include <memory>
#include <vector>
#include <bitset>
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
    //Reset stack pointer to top
    reg = {0};
    reg.SP = sizeof(memory) - 1;

    //Setup register lookup tables
    reg_table_r = {&reg.B, &reg.C, &reg.D, &reg.E, &reg.H, &reg.L, (uint8_t*)&reg.HL, &reg.A};
    reg_table_rp = {&reg.BC, &reg.DE, &reg.HL, &reg.SP};
    reg_table_rp2 = {&reg.BC, &reg.DE, &reg.HL, &reg.AF};

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
    for(uint64_t a = 0; a < data.size(); ++a)
    {
        //Get instruction prefix
        Prefix prefix = to_prefix(data[a]);
        if(prefix != Prefix::None)
            ++a;

        uint8_t x, y, z, p, q;
        x = (data[a] >> 6) & 0x3;
        y = (data[a] >> 3) & 0x7;
        z = data[a] & 0x7;
        p = (y >> 1) & 0x3;
        q = (y >> 2) & 0x2;

        switch(prefix)
        {
            case Prefix::None:
            {
                switch(x)
                {
                    case 0:
                    {
                        switch(z)
                        {
                            case 0: // z = 0
                                switch(y)
                                {
                                    case 0: // NOP
                                        break;
                                    case 1:
                                        break;
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
                                        *reg_table_rp[p] = data[a + 1] | (data[a + 2] << 8);
                                        a += 2;

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
                                ++(*reg_table_r[y]);
                                log_stream << "INC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 5: // DEC r[y]
                            {
                                --(*reg_table_r[y]);
                                log_stream << "DEC " << reg_table_r_names[y] << std::endl;
                                break;
                            }
                            case 6: // z = 6, LD r[y], n
                            {
                                *reg_table_r[y] = data[++a];

                                log_stream << "LD " << reg_table_r_names[p].c_str() << ", " << (uint16_t)*reg_table_r[y] << std::endl;
                                break;
                            }
                            case 7:
                                break;
                            default:
                                abort();
                        }
                    }
                    case 1:
                        break;
                    case 2:
                        break;
                    case 3:
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
                                switch(y)
                                {
                                    case 0:
                                        break;
                                    case 1:
                                        break;
                                    case 2: // OUT (n), A
                                    {
                                        out(data[++a], &reg.A);
                                        log_stream << "OUT (" << (uint16_t)data[a] << "), A" << std::endl;
                                        break;
                                    }
                                    case 3: // IN A, (n)
                                    {
                                        in(data[++a], &reg.A);
                                        log_stream << "IN A, (" << (uint16_t)data[a] << ")" << std::endl;
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
