#include "progs.h"

std::span<Progs::ddef_t> Progs::GetFieldDefs()
{
    constinit static auto res = std::to_array({
        ddef_t{
            .type = etype_t::ev_void,
            .ofs = 0,
            .s_name = 0,
        },
ddef_t{
.type = etype_t::ev_float,
.ofs = 0,
.s_name = 41561,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 1,
.s_name = 41583,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 1,
.s_name = 41597,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 2,
.s_name = 41615,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 3,
.s_name = 41633,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 4,
.s_name = 41651,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 4,
.s_name = 41665,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 5,
.s_name = 41683,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 6,
.s_name = 41701,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 7,
.s_name = 41719,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 8,
.s_name = 41731,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 9,
.s_name = 41749,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 10,
.s_name = 41761,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 10,
.s_name = 41775,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 11,
.s_name = 41793,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 12,
.s_name = 41811,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 13,
.s_name = 41829,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 13,
.s_name = 41849,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 14,
.s_name = 41873,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 15,
.s_name = 41897,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 16,
.s_name = 41921,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 16,
.s_name = 41939,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 17,
.s_name = 41961,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 18,
.s_name = 41983,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 19,
.s_name = 42005,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 19,
.s_name = 42019,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 20,
.s_name = 42037,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 21,
.s_name = 42055,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 22,
.s_name = 42073,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 22,
.s_name = 42093,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 23,
.s_name = 42117,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 24,
.s_name = 42141,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 25,
.s_name = 42165,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 25,
.s_name = 42187,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 26,
.s_name = 42213,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 27,
.s_name = 42239,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 28,
.s_name = 42265,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 29,
.s_name = 42285,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 30,
.s_name = 42297,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 31,
.s_name = 42309,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 32,
.s_name = 42319,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 33,
.s_name = 42335,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 33,
.s_name = 42345,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 34,
.s_name = 42359,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 35,
.s_name = 42373,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 36,
.s_name = 42387,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 36,
.s_name = 42397,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 37,
.s_name = 42411,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 38,
.s_name = 42425,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 39,
.s_name = 42439,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 39,
.s_name = 42449,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 40,
.s_name = 42463,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 41,
.s_name = 42477,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 42,
.s_name = 42491,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 43,
.s_name = 42503,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 44,
.s_name = 42511,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 45,
.s_name = 42523,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 46,
.s_name = 42539,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 47,
.s_name = 42559,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 48,
.s_name = 42585,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 49,
.s_name = 42599,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 50,
.s_name = 42611,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 51,
.s_name = 42625,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 52,
.s_name = 42649,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 53,
.s_name = 42673,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 54,
.s_name = 42697,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 55,
.s_name = 42721,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 56,
.s_name = 42743,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 57,
.s_name = 42769,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 58,
.s_name = 42791,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 59,
.s_name = 42803,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 60,
.s_name = 42825,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 61,
.s_name = 42837,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 62,
.s_name = 42855,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 62,
.s_name = 42873,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 63,
.s_name = 42895,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 64,
.s_name = 42917,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 65,
.s_name = 42939,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 66,
.s_name = 42955,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 67,
.s_name = 42971,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 68,
.s_name = 42987,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 69,
.s_name = 43003,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 70,
.s_name = 43021,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 70,
.s_name = 43037,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 71,
.s_name = 43057,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 72,
.s_name = 43077,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 73,
.s_name = 43097,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 74,
.s_name = 43119,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 75,
.s_name = 43135,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 76,
.s_name = 43147,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 77,
.s_name = 43159,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 78,
.s_name = 43177,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 79,
.s_name = 43187,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 80,
.s_name = 43209,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 81,
.s_name = 43237,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 82,
.s_name = 43257,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 83,
.s_name = 43279,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 84,
.s_name = 43301,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 85,
.s_name = 43321,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 86,
.s_name = 43341,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 87,
.s_name = 43361,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 88,
.s_name = 43375,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 89,
.s_name = 43397,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 90,
.s_name = 43419,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 91,
.s_name = 43433,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 92,
.s_name = 43455,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 93,
.s_name = 43473,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 94,
.s_name = 43491,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 95,
.s_name = 43519,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 96,
.s_name = 43531,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 96,
.s_name = 43547,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 97,
.s_name = 43567,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 98,
.s_name = 43587,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 99,
.s_name = 43607,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 100,
.s_name = 43623,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 101,
.s_name = 43637,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 102,
.s_name = 43649,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 103,
.s_name = 43663,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 104,
.s_name = 43677,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 105,
.s_name = 45488,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 106,
.s_name = 45496,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 107,
.s_name = 45504,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 108,
.s_name = 45524,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 109,
.s_name = 45546,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 110,
.s_name = 45566,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 111,
.s_name = 45578,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 112,
.s_name = 45596,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 113,
.s_name = 45612,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 114,
.s_name = 45626,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 115,
.s_name = 45648,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 116,
.s_name = 45666,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 117,
.s_name = 45682,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 118,
.s_name = 45696,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 119,
.s_name = 45714,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 120,
.s_name = 45726,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 121,
.s_name = 45738,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 122,
.s_name = 45762,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 123,
.s_name = 45831,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 124,
.s_name = 45851,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 125,
.s_name = 45883,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 126,
.s_name = 45911,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 127,
.s_name = 45951,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 128,
.s_name = 45989,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 129,
.s_name = 46033,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 130,
.s_name = 46067,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 131,
.s_name = 46099,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 132,
.s_name = 46133,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 133,
.s_name = 46163,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 134,
.s_name = 46195,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 135,
.s_name = 46217,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 136,
.s_name = 46241,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 137,
.s_name = 46259,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 138,
.s_name = 46279,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 139,
.s_name = 46295,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 140,
.s_name = 46321,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 141,
.s_name = 46341,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 142,
.s_name = 46361,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 143,
.s_name = 46387,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 144,
.s_name = 46413,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 145,
.s_name = 46433,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 146,
.s_name = 46441,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 146,
.s_name = 46455,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 147,
.s_name = 46473,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 148,
.s_name = 46491,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 149,
.s_name = 46509,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 150,
.s_name = 46527,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 151,
.s_name = 46543,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 151,
.s_name = 46553,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 152,
.s_name = 46567,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 153,
.s_name = 46581,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 154,
.s_name = 46595,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 154,
.s_name = 46607,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 155,
.s_name = 46623,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 156,
.s_name = 46639,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 157,
.s_name = 46655,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 157,
.s_name = 46667,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 158,
.s_name = 46683,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 159,
.s_name = 46699,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 160,
.s_name = 46715,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 161,
.s_name = 46725,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 162,
.s_name = 46737,
},
ddef_t{
.type = etype_t::ev_string,
.ofs = 163,
.s_name = 46765,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 164,
.s_name = 46779,
},
ddef_t{
.type = etype_t::ev_entity,
.ofs = 165,
.s_name = 46799,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 166,
.s_name = 46821,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 167,
.s_name = 46833,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 168,
.s_name = 46841,
},
ddef_t{
.type = etype_t::ev_function,
.ofs = 169,
.s_name = 46849,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 170,
.s_name = 46863,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 170,
.s_name = 46883,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 171,
.s_name = 46907,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 172,
.s_name = 46931,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 173,
.s_name = 46955,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 173,
.s_name = 46977,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 174,
.s_name = 47003,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 175,
.s_name = 47029,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 176,
.s_name = 47055,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 177,
.s_name = 47067,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 178,
.s_name = 47075,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 179,
.s_name = 47087,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 179,
.s_name = 47097,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 180,
.s_name = 47111,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 181,
.s_name = 47125,
},
ddef_t{
.type = etype_t::ev_vector,
.ofs = 182,
.s_name = 47139,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 182,
.s_name = 47149,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 183,
.s_name = 47163,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 184,
.s_name = 47177,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 185,
.s_name = 47191,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 186,
.s_name = 47205,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 187,
.s_name = 47221,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 188,
.s_name = 47237,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 189,
.s_name = 47255,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 190,
.s_name = 51149,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 191,
.s_name = 51171,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 192,
.s_name = 54816,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 193,
.s_name = 58159,
},
ddef_t{
.type = etype_t::ev_float,
.ofs = 194,
.s_name = 74984,
},
        });
    return res;
}
