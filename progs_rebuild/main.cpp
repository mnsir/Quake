#include <string>
#include <string_view>
#include <span>
#include <array>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <format>
#include <set>
#include <variant>
#include <map>
#include <charconv>

#include <common/progs.h>

struct Tabs
{
    static inline int count = 2;

    static std::string gen() { return std::string(count, '\t'); }
};

std::string ToString(Progs::Op op)
{
    switch (op)
    {
    case Progs::Op::OP_DONE: return "Op::OP_DONE";
    case Progs::Op::OP_MUL_F: return "Op::OP_MUL_F";
    case Progs::Op::OP_MUL_V: return "Op::OP_MUL_V";
    case Progs::Op::OP_MUL_FV: return "Op::OP_MUL_FV";
    case Progs::Op::OP_MUL_VF: return "Op::OP_MUL_VF";
    case Progs::Op::OP_DIV_F: return "Op::OP_DIV_F";
    case Progs::Op::OP_ADD_F: return "Op::OP_ADD_F";
    case Progs::Op::OP_ADD_V: return "Op::OP_ADD_V";
    case Progs::Op::OP_SUB_F: return "Op::OP_SUB_F";
    case Progs::Op::OP_SUB_V: return "Op::OP_SUB_V";
    case Progs::Op::OP_EQ_F: return "Op::OP_EQ_F";
    case Progs::Op::OP_EQ_V: return "Op::OP_EQ_V";
    case Progs::Op::OP_EQ_S: return "Op::OP_EQ_S";
    case Progs::Op::OP_EQ_E: return "Op::OP_EQ_E";
    case Progs::Op::OP_EQ_FNC: return "Op::OP_EQ_FNC";
    case Progs::Op::OP_NE_F: return "Op::OP_NE_F";
    case Progs::Op::OP_NE_V: return "Op::OP_NE_V";
    case Progs::Op::OP_NE_S: return "Op::OP_NE_S";
    case Progs::Op::OP_NE_E: return "Op::OP_NE_E";
    case Progs::Op::OP_NE_FNC: return "Op::OP_NE_FNC";
    case Progs::Op::OP_LE: return "Op::OP_LE";
    case Progs::Op::OP_GE: return "Op::OP_GE";
    case Progs::Op::OP_LT: return "Op::OP_LT";
    case Progs::Op::OP_GT: return "Op::OP_GT";
    case Progs::Op::OP_LOAD_F: return "Op::OP_LOAD_F";
    case Progs::Op::OP_LOAD_V: return "Op::OP_LOAD_V";
    case Progs::Op::OP_LOAD_S: return "Op::OP_LOAD_S";
    case Progs::Op::OP_LOAD_ENT: return "Op::OP_LOAD_ENT";
    case Progs::Op::OP_LOAD_FLD: return "Op::OP_LOAD_FLD";
    case Progs::Op::OP_LOAD_FNC: return "Op::OP_LOAD_FNC";
    case Progs::Op::OP_ADDRESS: return "Op::OP_ADDRESS";
    case Progs::Op::OP_STORE_F: return "Op::OP_STORE_F";
    case Progs::Op::OP_STORE_V: return "Op::OP_STORE_V";
    case Progs::Op::OP_STORE_S: return "Op::OP_STORE_S";
    case Progs::Op::OP_STORE_ENT: return "Op::OP_STORE_ENT";
    case Progs::Op::OP_STORE_FLD: return "Op::OP_STORE_FLD";
    case Progs::Op::OP_STORE_FNC: return "Op::OP_STORE_FNC";
    case Progs::Op::OP_STOREP_F: return "Op::OP_STOREP_F";
    case Progs::Op::OP_STOREP_V: return "Op::OP_STOREP_V";
    case Progs::Op::OP_STOREP_S: return "Op::OP_STOREP_S";
    case Progs::Op::OP_STOREP_ENT: return "Op::OP_STOREP_ENT";
    case Progs::Op::OP_STOREP_FLD: return "Op::OP_STOREP_FLD";
    case Progs::Op::OP_STOREP_FNC: return "Op::OP_STOREP_FNC";
    case Progs::Op::OP_RETURN: return "Op::OP_RETURN";
    case Progs::Op::OP_NOT_F: return "Op::OP_NOT_F";
    case Progs::Op::OP_NOT_V: return "Op::OP_NOT_V";
    case Progs::Op::OP_NOT_S: return "Op::OP_NOT_S";
    case Progs::Op::OP_NOT_ENT: return "Op::OP_NOT_ENT";
    case Progs::Op::OP_NOT_FNC: return "Op::OP_NOT_FNC";
    case Progs::Op::OP_IF: return "Op::OP_IF";
    case Progs::Op::OP_IFNOT: return "Op::OP_IFNOT";
    case Progs::Op::OP_CALL0: return "Op::OP_CALL0";
    case Progs::Op::OP_CALL1: return "Op::OP_CALL1";
    case Progs::Op::OP_CALL2: return "Op::OP_CALL2";
    case Progs::Op::OP_CALL3: return "Op::OP_CALL3";
    case Progs::Op::OP_CALL4: return "Op::OP_CALL4";
    case Progs::Op::OP_CALL5: return "Op::OP_CALL5";
    case Progs::Op::OP_CALL6: return "Op::OP_CALL6";
    case Progs::Op::OP_CALL7: return "Op::OP_CALL7";
    case Progs::Op::OP_CALL8: return "Op::OP_CALL8";
    case Progs::Op::OP_STATE: return "Op::OP_STATE";
    case Progs::Op::OP_GOTO: return "Op::OP_GOTO";
    case Progs::Op::OP_AND: return "Op::OP_AND";
    case Progs::Op::OP_OR: return "Op::OP_OR";
    case Progs::Op::OP_BITAND: return "Op::OP_BITAND";
    case Progs::Op::OP_BITOR: return "Op::OP_BITOR";
    }
    return "";
}

std::string ToString(Progs::ddef_t::etype_t type)
{
    switch (type)
    {
    case Progs::ddef_t::etype_t::ev_void: return "ddef_t::etype_t::ev_void";
    case Progs::ddef_t::etype_t::ev_string: return "ddef_t::etype_t::ev_string";
    case Progs::ddef_t::etype_t::ev_float: return "ddef_t::etype_t::ev_float";
    case Progs::ddef_t::etype_t::ev_vector: return "ddef_t::etype_t::ev_vector";
    case Progs::ddef_t::etype_t::ev_entity: return "ddef_t::etype_t::ev_entity";
    case Progs::ddef_t::etype_t::ev_field: return "ddef_t::etype_t::ev_field";
    case Progs::ddef_t::etype_t::ev_function: return "ddef_t::etype_t::ev_function";
    }
    return "";
}

struct ItemOuter
{
    ItemOuter(std::ostream& os, std::string in, std::string out, bool newlines)
        : os_(os)
        , out_(out)
        , newlines_(newlines)
    {
        os_ << Tabs::gen() << in << (newlines_ ? "\n" : " ");
        Tabs::count++;
    }
    ~ItemOuter()
    {
        Tabs::count--;
        if (newlines_)
            os_ << Tabs::gen();
        os_ << out_ << "\n";
    }
    std::ostream& os_;
    std::string out_;
    bool newlines_;
};

using VarVector = std::array<float, 3>;
struct VarString { int data; };
struct VarFunction { int data; };
struct VarEntity { int data; };
using ProgsVariant = std::variant<std::monostate, float, VarVector, VarString, VarFunction, VarEntity, int>;

ProgsVariant ToData(Progs::ddef_t::etype_t type, int offset)
{
    auto&& gl = Progs::GetGlobalStruct();

    auto* arr = reinterpret_cast<int*>(&gl);
    auto* arrf = reinterpret_cast<const float*>(&gl) + offset;
    const auto& item = arr[offset];

    switch (type)
    {
    case Progs::ddef_t::etype_t::ev_void:
        return {};
    case Progs::ddef_t::etype_t::ev_string:
        return VarString{ .data = item };
    case Progs::ddef_t::etype_t::ev_float:
        return arrf[0];
    case Progs::ddef_t::etype_t::ev_vector:
        return VarVector{ arrf[0], arrf[1], arrf[2] };
    case Progs::ddef_t::etype_t::ev_entity:
        return VarEntity{ .data = item };
    case Progs::ddef_t::etype_t::ev_field:
        return {};
    case Progs::ddef_t::etype_t::ev_function:
        return VarFunction{ .data = item };
    }
    return {};
}

std::string ToString(ProgsVariant v)
{
    return std::visit([](auto&& v) {
        std::ostringstream os;
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>)
            os << "error";
        else if constexpr (std::is_same_v < T, float>)
        {
            std::array<char, 128> a{};
            auto&& [ptr, ec] = std::to_chars(a.data(), a.data() + a.size(), v, std::chars_format::fixed);
            if (ec == std::errc{})
            {
                os << "static_cast<float>(" << a.data() << ")";
            }
            else
            {
                os << "error";
            }
        }
        else if constexpr (std::is_same_v < T, VarVector>)
            os << "VarVector{ " << v[0] << ", " << v[1] << ", " << v[2] << " }";
        else if constexpr (std::is_same_v < T, VarString>)
            os << "VarString{ .data = " << v.data << " }";
        else if constexpr (std::is_same_v < T, VarFunction>)
            os << "VarFunction{ .data = " << v.data << " }";
        else if constexpr (std::is_same_v < T, VarEntity>)
            os << "VarEntity{ .data = " << v.data << " }";
        else if constexpr (std::is_same_v < T, int>)
            os << "error";
        else
            static_assert(false, "shit");
        return os.str();
        }, v);
}

struct StackVar
{
    int offset;
    std::string name;
    ProgsVariant data;
};
/*
int main()
{
    auto&& fs = Progs::GetFunctions();
    auto&& gds = Progs::GetGlobalDefs();


    std::ofstream out("out.txt");
    std::ofstream out2("out2.txt");
    std::set<int> skip_defs;

    auto tabbed_field_if_not_null = [&out](std::string name, int val) {
        if (val)
            out << Tabs::gen() << '.' << name << " = " << val << ",\n";
        };
    auto simple_field_if_not_null = [&out](std::string name, int val) {
        if (val)
            out << '.' << name << " = " << val << ", ";
        };

    for (size_t i = 0; i < fs.size(); ++i)
    {
        ItemOuter dfunc(out, "dfunction_t{", "},", true);
        auto&& curf = fs[i];

        int parm_start = curf.parm_start;
        int parm_end = i + 1 < fs.size() ? fs[i + 1].parm_start : 12000;
        std::set<int> offsets;
        auto check_and_add = [&offsets, parm_start, parm_end](int offset) {
            if (parm_start <= offset && offset < parm_end)
                offsets.insert(offset);
            };

        tabbed_field_if_not_null("first_statement", curf.first_statement);
        if (!curf.statements.empty())
        {
            ItemOuter dstatements(out, ".statements = {", "},", true);
            for (size_t j = 0; j < curf.statements.size(); ++j)
            {
                ItemOuter dstatements(out, "dstatement_t{", "},", false);
                auto&& st = curf.statements[j];
                out << ".op = " << ToString(curf.statements[j].op) << ", ";
                simple_field_if_not_null("a", st.a);
                simple_field_if_not_null("b", st.b);
                simple_field_if_not_null("c", st.c);

                check_and_add(st.a);
                check_and_add(st.b);
                check_and_add(st.c);
            }
        }
        tabbed_field_if_not_null("parm_start", curf.parm_start);
        //tabbed_field_if_not_null("parm_end", parm_end);
        {
            std::vector<StackVar> stack_vars;
            std::set<int> skip;
            for (auto&& offset : offsets)
            {
                std::string name;
                ProgsVariant data;
                if (auto it = std::ranges::find(gds, offset, &Progs::ddef_t::ofs); it != gds.end())
                {
                    if (it->type == Progs::ddef_t::etype_t::ev_field || it->type == Progs::ddef_t::etype_t::ev_void || it->save_global)
                    {
                        out << "error";
                    }

                    data = ToData(it->type, offset);
                    name = it->s_name;
                }
                if (!skip.contains(offset))
                {
                    stack_vars.push_back(StackVar{
                        .offset = offset,
                        .name = name,
                        .data = data,
                        });

                    skip.insert(offset);
                    if (std::holds_alternative<Progs::VarVector>(data))
                    {
                        skip.insert(offset + 1);
                        skip.insert(offset + 2);
                    }
                }
            }
            skip_defs.insert(skip.begin(), skip.end());

            //if (!stack_vars.empty())
            //{
            //    ItemOuter dvars(out, ".stack_vars = {", "},", true);
            //    for (auto&& var : stack_vars)
            //    {
            //        ItemOuter dvars(out, "StackVar{", "},", false);
            //        simple_field_if_not_null("offset", var.offset);
            //        if (!var.name.empty())
            //        {
            //            out << ".name = " << std::quoted(var.name) << "sv" << ", ";
            //            out << ".data = " << ToString(var.data) << ", ";
            //        }
            //    }
            //}
        }
        tabbed_field_if_not_null("locals", curf.locals);
        out << Tabs::gen() << ".s_name = " << std::quoted(curf.s_name) << "sv" << ",\n";
        out << Tabs::gen() << ".s_file = " << std::quoted(curf.s_file) << "sv" << ",\n";
        tabbed_field_if_not_null("numparms", curf.numparms);
        if (curf.numparms)
        {
            ItemOuter dparm_size(out, ".parm_size = {", "},", false);
            for (size_t j = 0; j < curf.numparms; ++j)
            {
                out << static_cast<int>(curf.parm_size[j]) << ", ";
            }
        }
    }

    for (auto&& g : gds)
    {
        if (!skip_defs.contains(g.ofs))
        {
            out2 << "ddef_t{ ";
            out2 << ".save_global = " << (g.save_global ? "true" : "false") << ", ";
            out2 << ".type = " << ToString(g.type) << ", ";
            out2 << ".ofs = " << g.ofs << ", ";
            out2 << ".s_name = " << g.s_name << " ";
            out2 << "},\n";
        }
    }

    return 0;
}
*/
/*
int main()
{
    std::map<Progs::Op, std::set<int>> m;
    auto&& fs = Progs::GetFunctions();
    auto&& dd = Progs::GetGlobalDefs();

    for (auto&& f : fs)
    {
        for (auto&& s : f.statements)
        {
            if (s.op == Progs::Op::OP_LOAD_F
                || s.op == Progs::Op::OP_LOAD_FLD
                || s.op == Progs::Op::OP_LOAD_ENT
                || s.op == Progs::Op::OP_LOAD_S
                || s.op == Progs::Op::OP_LOAD_FNC
                || s.op == Progs::Op::OP_LOAD_V)
            {
                if (auto it = std::ranges::find(dd, s.b, &Progs::ddef_t::ofs); it != dd.end())
                {
                    if (it->type != Progs::ddef_t::etype_t::ev_field)
                    {
                        int i = 0;
                        ++i;
                    }
                }
                else
                {
                    int i = 0;
                    ++i;
                }
                m[s.op].insert(s.b);
            }
        }
    }
    int i = 0;
    ++i;
}*/

std::string ToUpper(std::string str)
{
    for (auto& ch : str)
        ch = std::toupper(ch);
    return str;
}


int main()
{
    auto&& fs = Progs::GetFunctions();
    auto&& fds = Progs::GetFieldDefs();
    auto&& globals = (int*)&Progs::GetGlobalStruct();

    std::set<std::string> new_st;

    std::ofstream out("out.txt");

    for (size_t i = 0; i < fs.size(); ++i)
    {
        ItemOuter dfunc(out, "dfunction_t{", "},", true);
        auto&& curf = fs[i];

        if (curf.first_statement)
            out << Tabs::gen() << ".first_statement = " << curf.first_statement << ",\n";

        if (!curf.statements.empty())
        {
            ItemOuter dstatements(out, ".statements = {", "},", true);
            for (size_t j = 0; j < curf.statements.size(); ++j)
            {
                auto&& s = curf.statements[j];

                if (s.op == Progs::Op::OP_LOAD_F
                    || s.op == Progs::Op::OP_LOAD_FLD
                    || s.op == Progs::Op::OP_LOAD_ENT
                    || s.op == Progs::Op::OP_LOAD_S
                    || s.op == Progs::Op::OP_LOAD_FNC
                    || s.op == Progs::Op::OP_LOAD_V)
                {
                    int ofs = globals[s.b];
                    if (auto it = std::ranges::find(fds, ofs, &Progs::FieldDef::ofs); it != fds.end())
                    {
                        auto upper = ToUpper(std::string(it->name));
                        if (upper.empty())
                            upper = "MODELINDEX";

                        if (it->type == Progs::FieldDef::Type::ev_vector && s.op == Progs::Op::OP_LOAD_F)
                        {
                            upper += "_X";
                        }
                        //{
                        //    if (auto it = std::ranges::find_if(curf.statements, [c = s.c](auto&& s) { return s.a == c || s.b == c; }); it != curf.statements.end())
                        //    {
                        //        std::set<Progs::Op> f{};
                        //        std::set<Progs::Op> v{
                        //            Progs::Op::OP_EQ_V,
                        //            Progs::Op::OP_NE_V,
                        //            Progs::Op::OP_STORE_V,
                        //            Progs::Op::OP_STOREP_V,
                        //            Progs::Op::OP_SUB_V,
                        //            Progs::Op::OP_ADD_V,
                        //        };
                        //        auto op = it->op;
                        //        if (f.contains(op))
                        //        {
                        //            upper += "_X";
                        //        }
                        //        else if (v.contains(op))
                        //        {
                        //
                        //        }
                        //        else
                        //        {
                        //            int i = 0;
                        //            ++i;
                        //        }
                        //    }
                        //    else
                        //        throw std::exception("strange");
                        //}

                        auto op = "Op::LOAD_" + upper;
                        ItemOuter dstatement(out, "dstatement_t{", "},", false);
                        out << ".op = " << op << ", ";
                        out << ".a = " << s.a << ", ";
                        out << ".c = " << s.c << ", ";
                        new_st.insert(op);
                    }
                    else
                        throw std::exception("ewqe");
                }
                else
                {
                    ItemOuter dstatement(out, "dstatement_t{", "},", false);
                    out << ".op = " << ToString(s.op) << ", ";
                    out << ".a = " << s.a << ", ";
                    out << ".b = " << s.b << ", ";
                    out << ".c = " << s.c << ", ";
                }
            }
        }

        if (curf.parm_start)
            out << Tabs::gen() << ".parm_start = " << curf.parm_start << ",\n";

        if (curf.locals)
            out << Tabs::gen() << ".locals = " << curf.locals << ",\n";

        if (!curf.s_name.empty())
            out << Tabs::gen() << ".s_name = " << std::quoted(curf.s_name) << "sv" << ",\n";

        if (!curf.s_file.empty())
            out << Tabs::gen() << ".s_file = " << std::quoted(curf.s_file) << "sv" << ",\n";

        if (curf.numparms)
        {
            out << Tabs::gen() << ".numparms = " << curf.numparms << ",\n";
            {
                ItemOuter dparm_size(out, ".parm_size = {", "},", false);
                for (size_t j = 0; j < curf.numparms; ++j)
                {
                    out << static_cast<int>(curf.parm_size[j]) << ", ";
                }
            }
        }

    }

    std::ofstream out2("out2.txt");

    for (auto&& str : new_st)
    {
        out2 << str << std::endl;
    }

    return 0;
}

/*
int main()
{
    auto&& fs = Progs::GetFunctions();


    std::ofstream out("out.txt");

    for (size_t i = 0; i < fs.size(); ++i)
    {
        ItemOuter dfunc(out, "dfunction_t{", "},", true);
        auto&& curf = fs[i];

        if (curf.first_statement)
            out << Tabs::gen() << ".first_statement = " << curf.first_statement << ",\n";

        if (!curf.statements.empty())
        {
            ItemOuter dstatements(out, ".statements = {", "},", true);
            for (size_t j = 0; j < curf.statements.size(); ++j)
            {
                ItemOuter dstatements(out, "dstatement_t{", "},", false);
                out << ".op = " << ToString(curf.statements[j].op) << ", ";
                out << ".a = " << curf.statements[j].a << ", ";
                out << ".b = " << curf.statements[j].b << ", ";
                out << ".c = " << curf.statements[j].c << ", ";
            }
        }

        if (curf.parm_start)
            out << Tabs::gen() << ".parm_start = " << curf.parm_start << ",\n";

        if (curf.locals)
            out << Tabs::gen() << ".locals = " << curf.locals << ",\n";

        if (!curf.s_name.empty())
            out << Tabs::gen() << ".s_name = " << std::quoted(curf.s_name) << "sv" << ",\n";

        if (!curf.s_file.empty())
            out << Tabs::gen() << ".s_file = " << std::quoted(curf.s_file) << "sv" << ",\n";

        if (curf.numparms)
        {
            out << Tabs::gen() << ".numparms = " << curf.numparms << ",\n";
            {
                ItemOuter dparm_size(out, ".parm_size = {", "},", false);
                for (size_t j = 0; j < curf.numparms; ++j)
                {
                    out << static_cast<int>(curf.parm_size[j]) << ", ";
                }
            }
        }

    }

    return 0;
}
*/
