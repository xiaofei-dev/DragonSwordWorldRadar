using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Reflection.Emit;
using System.Text;

public static class DSILInspector
{
    private static readonly Dictionary<short, OpCode> Codes = BuildCodes();
    private static Dictionary<short, OpCode> BuildCodes()
    {
        var result = new Dictionary<short, OpCode>();
        foreach (FieldInfo field in typeof(OpCodes).GetFields(BindingFlags.Public | BindingFlags.Static))
        {
            if (field.FieldType == typeof(OpCode))
            {
                OpCode code = (OpCode)field.GetValue(null);
                result[code.Value] = code;
            }
        }
        return result;
    }
    private static int ReadI4(byte[] il, ref int p) { int v = BitConverter.ToInt32(il,p); p += 4; return v; }
    private static long ReadI8(byte[] il, ref int p) { long v = BitConverter.ToInt64(il,p); p += 8; return v; }
    private static string ResolveToken(MethodBase method, int token, OperandType type)
    {
        try
        {
            Module module = method.Module;
            Type[] typeArgs = method.DeclaringType != null && method.DeclaringType.IsGenericType ? method.DeclaringType.GetGenericArguments() : Type.EmptyTypes;
            Type[] methodArgs = method.IsGenericMethod ? method.GetGenericArguments() : Type.EmptyTypes;
            if (type == OperandType.InlineString) return "\"" + module.ResolveString(token) + "\"";
            MemberInfo member = module.ResolveMember(token,typeArgs,methodArgs);
            return member == null ? ("token=0x" + token.ToString("X8")) : member.ToString();
        }
        catch { return "token=0x" + token.ToString("X8"); }
    }
    public static string Disassemble(MethodBase method)
    {
        StringBuilder sb = new StringBuilder();
        sb.AppendLine(method.DeclaringType.FullName + "." + method.Name + " " + method.ToString());
        MethodBody body = method.GetMethodBody();
        if (body == null) { sb.AppendLine("<no method body>"); return sb.ToString(); }
        byte[] il = body.GetILAsByteArray();
        int p = 0;
        while (p < il.Length)
        {
            int offset = p;
            short value = il[p++];
            if (value == 0xFE) value = (short)(0xFE00 | il[p++]);
            OpCode code;
            if (!Codes.TryGetValue(value,out code)) { sb.AppendLine(offset.ToString("X4") + ": <unknown>"); break; }
            string operand = "";
            switch (code.OperandType)
            {
                case OperandType.InlineNone: break;
                case OperandType.ShortInlineI: operand = ((sbyte)il[p++]).ToString(CultureInfo.InvariantCulture); break;
                case OperandType.InlineI: operand = ReadI4(il,ref p).ToString(CultureInfo.InvariantCulture); break;
                case OperandType.InlineI8: operand = ReadI8(il,ref p).ToString(CultureInfo.InvariantCulture); break;
                case OperandType.ShortInlineR: operand = BitConverter.ToSingle(il,p).ToString(CultureInfo.InvariantCulture); p += 4; break;
                case OperandType.InlineR: operand = BitConverter.ToDouble(il,p).ToString(CultureInfo.InvariantCulture); p += 8; break;
                case OperandType.ShortInlineBrTarget: { sbyte d=(sbyte)il[p++]; operand=(p+d).ToString("X4"); break; }
                case OperandType.InlineBrTarget: { int d=ReadI4(il,ref p); operand=(p+d).ToString("X4"); break; }
                case OperandType.InlineSwitch:
                    int n=ReadI4(il,ref p); int basePos=p+n*4; StringBuilder sw=new StringBuilder();
                    for(int i=0;i<n;i++){ int d=ReadI4(il,ref p); if(i>0)sw.Append(','); sw.Append((basePos+d).ToString("X4")); }
                    operand=sw.ToString(); break;
                case OperandType.InlineVar: operand=BitConverter.ToUInt16(il,p).ToString(CultureInfo.InvariantCulture); p+=2; break;
                case OperandType.ShortInlineVar: operand=il[p++].ToString(CultureInfo.InvariantCulture); break;
                case OperandType.InlineString:
                case OperandType.InlineField:
                case OperandType.InlineMethod:
                case OperandType.InlineType:
                case OperandType.InlineTok:
                case OperandType.InlineSig:
                    int token=ReadI4(il,ref p); operand=ResolveToken(method,token,code.OperandType); break;
                default: operand="<unsupported operand>"; break;
            }
            sb.Append(offset.ToString("X4")).Append(": ").Append(code.Name);
            if (operand.Length > 0) sb.Append(' ').Append(operand);
            sb.AppendLine();
        }
        return sb.ToString();
    }
}
