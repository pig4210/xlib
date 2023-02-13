64 A3 00000000
FF 15 <^D core@HttpClient::curlGlobalInit>
6A <B sizeof(JDWBCore)>
E8 <^F core@operator_new>
8B F0
89 75 .
68 <W sizeof(CoreContext)>0000
C7 45 FC 00000000
E8 <^F core@operator_new>
83 C4 08
89 45 .
8B C8
C6 45 FC 01

# 64 A3 00 00 00 00 FF 15 ?? ?? ?? ?? 6A ?? E8 ?? ?? ?? ?? 8B F0 89 75 ?? 68 ?? ?? 00 00 C7 45 FC 00 00 00 00 E8 ?? ?? ?? ?? 83 C4 08 89 45 ?? 8B C8 C6 45 FC 01

/

64 A3 00000000
FF 15 <^D core@HttpClient::curlGlobalInit>
6A 08               # sizeof(JDWBCore)
E8 <^F core@operator_new>
8B F0
89 75 .
68 88030000         # sizeof(CoreContext)
C7 45 FC 00000000
E8 <^F core@operator_new>
83 C4 08
89 45 .
8B C8
C6 45 FC 01
/

83 C4 0C
6A 1B
68 ....             # u8"成功转给其他同事！"
8B C8
C7 45 FC 00000000
E8 <^F std::string::append_pchar_size>
C7 45 . 00000000
.{ 20, 40 }
C6 45 FC 01
8B 48 .
8B 01
FF 50 <B ChatLogicPrivate::LocalSysMsg>
.{ 18, 40 }
75 08
8B 01
FF .  .
FF 50 <B IChatDelegate::close>

# 83 C4 0C 6A 1B 68 ?? ?? ?? ?? 8B C8 C7 45 FC 00 00 00 00 E8 ?? ?? ?? ?? C7 45 ?? 00 00 00 00

/

83 C4 0C
6A 1B
68 ....             # u8"成功转给其他同事！"
8B C8
C7 45 FC 00000000
E8 <^F std::string::append_pchar_size>
C7 45 . 00000000
.{ 20, 40 }
C6 45 FC 01
8B 48 .
8B 01
FF 50 28            # ChatLogicPrivate::LocalSysMsg
.{ 18, 40 }
75 08
8B 01
FF .  .
FF 50 0C            # IChatDelegate::close
