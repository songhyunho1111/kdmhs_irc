<h1 align="center">KDMHS-IRC</h1>

<p align="center">
  <em>A simple IRC server for programming class</em>
</p>

# Building
Build with **Winsock**:
```bash
git clone https://github.com/songhyunho1111/kdmhs_irc.git
cd kdmhs_irc
gcc main.c -o main.exe -lws2_32;
```

Start server:
```bash
.\main.exe --server
```

Start client:
```bash
.\main.exe
```
