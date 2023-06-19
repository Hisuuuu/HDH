/*Ho va ten: Le Trung Hoa Hieu
MSSV: 2115208
Lop: CTK45A
Ten bai tap: Xuat noi dung cua mot thu muc bat ky tren dia*/

#include <iostream.h>
#include <conio.h>
#include <iomanip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloc.h>
#include <dos.h>

#pragma region Structs

// BIOS Parameter Block (BPB)
typedef struct
{
    char jmp[3];      // The first byte to JMP SHORT XX NOP
    char Ver[8];      // OEM identifier
    unsigned SecSiz;  // Bytes/sector (in little-endian format)
    char ClustSiz;    // Sectors/cluster
    unsigned ResSec;  // Number of reserved sectors (included Boot Record Sectors)
    char FatCnt;      // Number of File Allocation Tables. Often 2
    unsigned RootSiz; // Max # of dictionary entries
    unsigned TotSec;  // Total logical sectors
    char Media;       // Indicates the media descriptor type (FAT ID)
    unsigned FatSiz;  // Number of sectors per FAT
    unsigned TrkSec;  // Number of sectors per track
    unsigned HeadCnt; // Number of heads or sides on the storage media
    unsigned HidSec;  // Number of hidden sectors (LBA)
} EntryBPB;

// Union between Sector array and BPB structure
typedef union
{
    char Sec[512];
    EntryBPB Entry;
} UnionBPB;

// The file time
typedef struct
{
    unsigned S : 5; // Seconds
    unsigned M : 6; // Minutes
    unsigned H : 5; // Hours
} Time;

// Union between Creation time and Time structure
typedef union
{
    unsigned intTime;
    Time T;
} UnionTime;

// The file date
typedef struct
{
    unsigned D : 5; // Days
    unsigned M : 4; // Months
    unsigned Y : 7; // Years
} Date;

// Union between Creation date and Date structure
typedef union
{
    unsigned intDate;
    Date Dat;
} UnionDate;

// Attribute Byte (8 bits)
typedef struct
{
    char ReadOnly : 1; // Indicates that the file is read only
    char Hidden : 1;   // Indicates a hidden file
    char System : 1;   // Indicates a system file
    char Volume : 1;   // Indicates a special entry containing the disk's volume label
    char SubDir : 1;   // Describes a subdirectory
    char Archive : 1;  // This is the archive flag
    char DR : 2;
} Attribute;

// Union between Attribute char and Attribute structure
typedef union
{
    char charAtt;
    Attribute Attr;
} UnionAttribute;

// Regular Directory Entry
typedef struct
{
    char FileName[8];   // The file name, the first byte indicates it's status
    char Ext[3];        // The file name extension
    UnionAttribute Att; // The file attributes
    char DR[10];
    UnionTime Tg;   // The file time
    UnionDate Ng;   // The file date
    unsigned Clust; // The starting cluster number
    long FileSize;  // The file size
} EntryDir;

// Union between Entry char array and Entry structure
typedef union
{
    char Entry[32];
    EntryDir Entdir;
} UnionDir;

// Define a node in linked list
typedef struct Node
{
    void *Data; // Content of node
    Node *Next; // The pointer of next node
} NodeType;

// Define a linked list
typedef NodeType *PointerType;

#pragma endregion

#pragma region GlobalScope

char *path;         // Duong dan
char drive;         // O dia
EntryBPB BPB;       // Bang tham so dia
unsigned char *FAT; // Noi dung bang FAT

#pragma endregion

#pragma region Functions

/*Chen mot nut vao danh sach
Tham so dau vao:
- list: Danh sach lien ket
- lastItem: Phan tu cuoi cung cua danh sach
- item: Phan tu can chen
Tra ve:
- true: Thanh cong
- false: That bai
*/
int InsertLast(PointerType &list, PointerType &lastItem, void *item)
{
    PointerType temp = new NodeType;
    if (!temp) // Kiem tra viec cap phat bo nho
    {
        cout << "\nLoi cap phat bo nho!";
        return 0;
    }
    temp->Data = item; // Gan gia tri cua item cho con tro Data cua temp
    temp->Next = NULL;
    if (list == NULL) // Kiem tra danh sach lien ket
        list = temp;
    else
        lastItem->Next = temp;
    lastItem = temp;
    return 1;
}

/*So sanh hai chuoi
Tham so dau vao:
- string1: Chuoi ban dau
- string2: Chuoi thu hai
Tra ve:
- true: bang nhau
- false: khong bang nhau
*/
int Compare(char *string1, char *string2)
{
    for (int i = 0; string1[i] != '\0'; i++) // Duyet qua tung ky tu cua string1
        if (string1[i] != string2[i]) // Neu co ky tu trong string1 khac string2
            return 0;
    if (string2[i] == ' ') // Kiem tra ky tu tiep theo co phai la khoang trang hay khong
        return 1;
    else
        return 0;
}

/*Phan tich duong dan va trich xuat ten tep/thu muc
Tham so dau vao:
- path: Chuoi duong dan ma nguoi dung nhap vao
Tra ve:
- NULL: Duong dan khong hop le
- !NULL: Danh sach duong dan
*/
PointerType AnalysePath(char *path)
{
    PointerType list = NULL, last = NULL; // Khai bao con tro den nut dau tien va cuoi cung cua danh sach lien ket
    char *fileName; // Luu ten tep/thu muc trong duong dan
    drive = path[0]; // Trich xuat chu cai o dia bang ky tu dau tien
    if (drive >= 97 && drive <= 122) //Neu la chu thuong tru di 97 de thanh chu hoa, chuyen gia tri chu thanh gia tri so tuong ung
        drive -= 97; 
    else if (drive >= 65 && drive <= 90) // Neu la chu hoa tru 65, tuong tu
        drive -= 65; 
    else // Neu ky tu dau khong phai la chu cai
    {
        cout << "\nKhong co o dia " + drive;
        return NULL;
    }
    if (drive == 2)
        drive = 0x80; //Gia tri o C
    if (drive == 0 || drive == 1 || drive == (char)0x80) // Chi chap nhan o A, B, C
    {
        if (path[1] != ':' && path[2] != '\\') // Kiem tra ky tu tiep theo co phai la :\  //
        {
            cout << "\nDuong dan nhap sai";
            return NULL;
        }
        if (path[3] == '\0') // Kiem tra null
        {
            cout << "\nDuong dan chua co thu muc";
            return NULL;
        }
        int i = 3; // Ky tu con lai
        while (path[i] != '\0') // Doc den het duong dan
        {
            int j = 0;
            fileName = new char[12];
            if (!fileName)
            {
                cout << "\nLoi cap phat bo nho cho ten file";
                return NULL;
            }
            while (path[i] != '\\' && path[i] != '\0') // Doc den het ten cua thu muc
            {
                fileName[j] = path[i];
                ++i;
                ++j;
            }
            fileName = strupr(fileName); // Doi sang chu hoa
            fileName[j] = '\0';
            j = 0;
            while (fileName[j] != '.' && fileName[j] != '\0') // Lay ten cua thu muc (khong tinh phan mo rong)
                ++j;
            if (fileName[j] == '.') // Neu no la tap tin, chuyen doi thanh quy uoc dat ten 8.3 dung boi MS-DOS
            {
                fileName[j] = ' ';
                ++j;
                int k = 3;
                while (fileName[j] != '\0')
                {
                    fileName[11 - k] = fileName[j];
                    ++j;
                }
                fileName[11] = '\0';
                k = 7;
                while (fileName[k] != ' ')
                {
                    fileName[k] = ' ';
                    k--;
                }
            }
            if (path[i] != '\0')
                i++;
            InsertLast(list, last, fileName);
        }
        return list;
    }
    else
    {
        cout << "\nDuong dan thieu o dia!";
        return NULL;
    }
}

/*Tim kiem thu muc trong danh sach cac entry cua thu muc
Tham so dau vao:
- listEntry: Danh sach entry cua mot thu muc (goc hoac con)
- fileName: Ten cua thu muc con
- dir: Thu muc tim duoc (ket qua)
Tra ve:
- true: Tim thay
- false: Khong tim thay
*/
int SearchDir(PointerType listEntry, char *fileName, EntryDir &dir)
{
    PointerType p = listEntry; 
    while (p != NULL) // Duyet toan bo entry cua thu muc
    {
        if (Compare(fileName, ((EntryDir *)p->Data)->FileName))
        {
            dir = *(EntryDir *)p->Data;
            return 1;
        }
        p = p->Next;
    }
    return 0;
}

/*Doc cac sector cua o dia luu vao buffer bang ham ngat 13 cua BIOS
Tham so dau vao
- buff: Con tro toi buffer de luu du lieu doc vao
- side: So mat cua dia
- track: So track cua dia
- sector: Vi tri sector
- number: So luong sector can doc
Tra ve:
- true: Thanh cong
- false: Khong thanh cong
*/
int ReadDiskBIOS(char *buff, unsigned side, unsigned track, unsigned sector, unsigned number)
{
    union REGS u, v; // Thanh ghi da dung
    struct SREGS s;  // Thanh ghi doan
    int i = 0;    // i: So lan doc toi da
    v.x.cflag = 1;   // Dat co carry trong thanh ghi v thanh 1, de kiem tra loi khi doc dia
    while (i < 2 && v.x.cflag != 0) 
    {
        //Chi dinh cac tham so can thiet de doc tu dia
        u.h.ah = 0x2;   // Doc dia - Neu la 0x1: Ghi dia
        u.h.dl = drive; // O dia - Neu la 80: Dia cung
        u.h.dh = side;            // Side
        u.h.cl = sector;          // Sector bat dau doc du lieu
        u.h.ch = track;           // Track
        u.x.cx = track;           // Track bat dau doc du lieu
        u.h.al = number;          // So luong sector can doc
        s.es = FP_SEG(buff);      // Vi tri buff luu du lieu duoc doc
        u.x.bx = FP_OFF(buff);    // Dia chi offset cua buff
        int86x(0x13, &u, &v, &s); // Ham ngat 13h
        i++;                      // Tang so lan doc
    }
    return !(v.x.cflag); // Kiem tra loi
}

/*Doc bang tham so dia BPB
 */
void ReadBPB()
{
    UnionBPB temp;
    if (drive == 0 || drive == 1) //Xac dinh o dia doc la dia mem hay dia cung
        if (!ReadDiskBIOS(temp.Sec, 0, 1, 1, 1)) // Chi dinh sector dau tien chua BPB tren dia
        {
            cout << "\nKhong doc duoc bang tham so dia";
            return;
        }
    BPB = temp.Entry;
}

/*Doi cac gia tri trong phan vung thanh cac thong so side, track, sector tuong ung
Tham so dau vao:
- start: Vi tri bat dau
- side: So mat cua dia
- track: So track cua dia
- sector: Vi tri sector
Gia tri track moi la ket hop giua gia tri sector 
Luu y: Phai gan cho bien toan cuc BPB truoc
*/
void Change(long start, unsigned &side, unsigned &track, unsigned &sector)
{
    unsigned x;
    sector = (unsigned)(1 + start % BPB.TrkSec);
    side = (unsigned)((start / BPB.TrkSec) % BPB.HeadCnt);
    track = (unsigned)(start / (BPB.TrkSec * BPB.HeadCnt));
    x = track; // Gia tri track ban dau
    x = x & 0xFF00; // Lam tron den 8 bit cao nhat
    x = x >> 2; // Dich sang phai 2 bit
    x = x & 0x00FF; // Lam tron 8 bit thap nhat
    x = x | sector;  // Ket hop voi gia tri cua sector de tao ra gia tri moi cho track
    track = track << 8; // Dich sang trai 8 bit
    track = track | x;
}

/*Doc du lieu tu dia
Tham so dau vao:
- buff: Buff luu du lieu doc duoc
- start: Vi tri bat dau doc
- number: So sector can doc
Tra ve:
- true: Thanh cong
- false: That bai
*/
int ReadDisk(char *buff, long start, int number)
{
    unsigned side, track, sector;
    Change(start, side, track, sector);
    if (ReadDiskBIOS(buff, side, track, sector, number))
        return 1;
    else
        return 0;
}

/*Doc noi dung bang FAT cho vao bien toan cuc
Luu y: Phai gan gia tri cho bien toan cuc BPB truoc
*/
int ReadFAT()
{
    FAT = new char[BPB.FatSiz * BPB.SecSiz];
    if (!FAT)
    {
        cout << "\nLoi cap phat bo nho!";
        return 0;
    }
    if (!ReadDisk(FAT, BPB.ResSec, BPB.FatSiz)) // Doc noi dung trong bang FAT
    {
        cout << "\nKhong doc duoc bang tham so dia!";
        return 0;
    }
    return 1;
}

/*Tim va tra ve entry tiep theo trong bang FAT
Tham so dau vao:
- index: Entry hien tai trong bang FAT
Tra ve: Vi tri cluster tiep theo cua index
Luu y: Phai gan ket qua cho bien toan cuc FAT truoc
*/
unsigned NextEntry(unsigned index)
{
    unsigned add, x, t;
    if (drive == 0 || drive == 1) // Neu o dia hien tai la dia mem (A, B)
    {
        add = index * 3 / 2; // Tinh toan vi tri byte dau tien cua 2 entry lien tiep
        x = FAT[add]; // Luu gia tri cua entry trong byte thu nhat
        t = FAT[add + 1]; // Luu gia tri cua entry trong byte lien tiep
        t = t << 8; // Dich sang trai 8 bit
        x = x + t; // Tao ra gia tri entry tiep theo
        if ((index % 2) == 0)
            x = x & 0x0FFF; // Neu la chan, lam tron xuong 12 bit thap nhat
        else
            x = x >> 4; // Neu la le, dich sang phai 4 bit
    }
    else // Neu khong phai la dia mem
    {
        add = (index * 2) - 1; // Vi tri byte dau tien cua entry tiep theo
        //Xu ly tuong tu dia mem
        x = FAT[add];
        t = FAT[add + 1];
        t = t << 8;
        x = x + t;
    }
    return x;
}

/*Lay danh sach cac cluster trong bang FAT chua phan vung root
Tra ve: Danh sach cluster
Luu y: Phai gan ket qua cho bien toan cuc BPB truoc
*/
PointerType GetClusterRoot()
{
    PointerType listCluster = NULL, last = NULL;
    unsigned rootSec; //Luu vi tri sector dau tien trong phan vung root
    unsigned *cluster;
    unsigned number; // So luong sector su dung de luu tru cac entry trong phan vung root
    rootSec = BPB.ResSec + BPB.FatSiz * (int)BPB.FatCnt;
    number = BPB.RootSiz * 32 / 512;
    for (int i = 0; i < number; i++, rootSec++)
    {
        cluster = new unsigned;
        *cluster = rootSec;
        InsertLast(listCluster, last, (unsigned *)cluster);
    }
    return listCluster;
}

/*Lay danh sach cac cluster lien tiep trong bang FAT
Tham so dau vao:
- start: Vi tri cluster bat dau
Tra ve: Danh sach cluster
*/
PointerType GetCluster(unsigned start)
{
    PointerType listCluster = NULL, last = NULL;
    unsigned *cluster, next = start;
    while(next >= 2 && next < 0xFEF ) // 2 la entry dau tien cua cluster, 0xFEF la entry cuoi cung
    {
        cluster = new unsigned;
        *cluster = next;
        InsertLast(listCluster, last, (unsigned *)cluster);
        next = NextEntry(*cluster);
    }
    return listCluster;
}

/*Lay danh sach cac entry trong phan vung
Tham so dau vao:
- listCluster: Danh sach cluster
- flag:		0 : Thu muc root
            1 : Thu muc con
Tra ve:
- NULL: Khong thanh cong
- !NULL: Danh sach entry cua thu muc
Luu y: Phai gan gia tri cho bien toan cuc BPB, drive truoc
*/
PointerType GetEntryDir(PointerType listCluster, char flag)
{
    PointerType p = listCluster, listEntry = NULL, last = NULL;
    unsigned size = BPB.ClustSiz * 512; // Kich thuoc cluster
    unsigned currentCluster, currentSector; // Cluster hien tai, Sector dau tien cua cluster hien tai
    unsigned char *buffDir = new unsigned char[size]; // Khoi tao buffer co kich thuoc la size
    UnionDir *dir; // Du lieu cac entry doc duoc
    while (p != NULL)
    {
        currentCluster = *(unsigned *)p->Data;
        if (flag)
            currentSector = BPB.ResSec + BPB.FatSiz * BPB.FatCnt + (BPB.RootSiz * 32) / 512 
            + (currentCluster - 2) * BPB.ClustSiz;
        else
            currentSector = currentCluster;
        if (!ReadDisk(buffDir, currentSector, BPB.ClustSiz))
        {
            cout << "\nKhong doc duoc dia!";
            return NULL;
        }
        for (int i = 0; i < size;)
        {
            if (buffDir[i] != (char)0x00)     // Kiem tra co phai entry rong
                if (buffDir[i] != (char)0xE5) // Neu entry khong bi xoa
                {
                    int j = 0;
                    dir = new UnionDir;
                    for (; j < 32; j++, i++)
                        dir->Entry[j] = buffDir[i];
                    InsertLast(listEntry, last, &dir->Entdir);
                }
                else
                    i += 32;
            else
                break;
        }
        p = p->Next;
    }
    delete buffDir;
    return listEntry;
}

/*In ra thong tin cua cac entry trong danh sach
Tham so dau vao:
- listEntry: Danh sach entry can hien thi thong tin
*/
void PrintTo(PointerType listEntry)
{
    PointerType p = listEntry;
    EntryDir dir;
    cout << '\n'
         << setiosflags(ios::left)
         << setw(8) << "Name"
         << setw(8) << "Ext"
         << setw(16) << "Size (bytes)"
         << setw(8) << "Time"
         << setw(12) << "Date";
    while (p != NULL)
    {
        dir = *(EntryDir *)p->Data;
        if (dir.Att.Attr.Volume == 0)
        {
            int i;
            cout << "\n";
            for (i = 0; i < 8; i++)
                cout << dir.FileName[i];
            for (i = 0; i < 3; i++)
                cout << dir.Ext[i];
            cout << '\t' << dir.FileSize
                 << "\t\t" << ((dir.Tg).T).H << ':' << ((dir.Tg).T).M
                 << '\t' << ((dir.Ng).Dat).D << '/' << ((dir.Ng).Dat).M 
                 << "/" << ((dir.Ng).Dat).Y + 1980;
        }
        p = p->Next;
    }
}

#pragma endregion

#pragma region Main

void ChayChuongTrinh()
{
    path = new char[256];
    cout << "\nNhap duong dan: ";
    scanf("%s", path);
    PointerType listPath = AnalysePath(path);
    if (listPath == NULL)
        return;
    ReadBPB(); // Doc bang BPB cua phan vung dang xet
    if (!ReadFAT()) // Doc bang FAT cua phan vung dang xet
        return;
    // Tai thu muc goc
    PointerType listCluster = GetClusterRoot();
    PointerType listEntry = GetEntryDir(listCluster, 0);
    // Tai thu muc con
    while (listPath)
    {
        EntryDir dir;
        if (SearchDir(listEntry, (char *)listPath->Data, dir))
        {
            listCluster = GetCluster(dir.Clust);
            listEntry = GetEntryDir(listCluster, 1);
        }
        else
        {
            cout << "\nKhong tim thay thu muc " << (char *)listPath->Data;
            return;
        }
        listPath = listPath->Next;
    }
    PrintTo(listEntry);
}

int main()
{
    char key;
    clrscr();
    do
    {
        ChayChuongTrinh();
        cout<<"\nNhan ESC de thoat khoi chuong trinh.";
        key = getch();
    } while (key != 27); //27 la ma ASCII cua phim ESC
    return 1;
}

#pragma endregion