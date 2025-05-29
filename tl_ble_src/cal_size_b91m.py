import sys
file = open(sys.argv[1],'r',encoding='UTF-8')
content = file.readlines()
print("\t\n")
SectionStart = 0
############## 统计RAM size和 Ret_Size #######################
IRAM_Size = 0
DRAM_Size = 0
Flash_Size = 0
Ret_Size = 0
for i in content:
    if i[0:9] == 'Sections:':
        SectionStart = 1
        continue
    if SectionStart == 0:
        continue
    lineSplit = i.split()
    if len(lineSplit)==12:
        if lineSplit[1] == '.exec.itable':
            size = (int(lineSplit[3],16) + int(lineSplit[2],16) + 3)//4*4  # 4字节对齐
            Ret_Size += size
    if i[0:9] == 'Sections:':
        break
        


################统计各个段的大小#################
vectors_size = 0  #done
retention_reset_size = 0  #done
aes_data_size = 0  #done
retention_data_size = 0  #done
ram_code_size = 0    #done
exec_itable_size = 0   #done
iram_noinit_data_size = 0  #done
iram_bss_size = 0  #done
text_size = 0   #done
rodata_size = 0 #done
eh_frame_size = 0   #done
data_size = 0   #done
sbss_size = 0   #done
bss_size = 0    #done   
sdk_version_size = 0  #done
print("************calculate size of each sections***********************\t")

for i in content:
    if i[0:9] == 'Sections:':
        SectionStart = 1
        continue
    if SectionStart == 0:
        continue
    lineSplit = i.split()
    if len(lineSplit) == 7:
        if lineSplit[1] == '.sdk_version':
            size = (int(lineSplit[2],16)+3)//4*4
            sdk_version_size += size
    #if len(lineSplit) == 12:
        if lineSplit[1] == '.vectors':
            size = (int(lineSplit[2],16)+3)//4*4
            vectors_size += size
        if lineSplit[1] == '.retention_reset':
            size = (int(lineSplit[2],16)+3)//4*4
            retention_reset_size += size
        if lineSplit[1] == '.exec.itable':
            size = (int(lineSplit[2],16)+3)//4*4
            exec_itable_size += size
        if lineSplit[1] == '.text':
            size = (int(lineSplit[2],16)+7)//8*8
            text_size = text_size + size
        if lineSplit[1] == '.rodata':
            size = (int(lineSplit[2],16)+7)//8*8
            rodata_size = rodata_size + size
        if lineSplit[1] == '.eh_frame':
            size = (int(lineSplit[2],16)+3)//4*4
            eh_frame_size = eh_frame_size + size
    #if len(lineSplit) == 8:
        if lineSplit[1] == '.aes_data':
            size = (int(lineSplit[2],16)+3)//4*4
            aes_data_size += size
        if lineSplit[1] == '.iram_noinit_data':
            size = (int(lineSplit[2],16)+3)//4*4
            iram_noinit_data_size += size
        if lineSplit[1] == '.iram_bss':
            size = (int(lineSplit[2],16)+3)//4*4
            iram_bss_size += size
        if lineSplit[1] == '.sbss':
            size = (int(lineSplit[2],16)+3)//4*4
            sbss_size += size
        if lineSplit[1] == '.bss':
            size = (int(lineSplit[2],16)+3)//4*4
            bss_size += size
    #if len(lineSplit) == 11:
        if lineSplit[1] == '.retention_data':
            size = (int(lineSplit[2],16)+7)//8*8
            retention_data_size += size
        if lineSplit[1] == '.ram_code':
            size = (int(lineSplit[2],16)+255)//256*256
            ram_code_size += size
        if lineSplit[1] == '.data':
            size = (int(lineSplit[2],16)+8)//8*8
            data_size += size
            
    if i[0:9] == 'Sections:':
        break

print("%-20s" % "vectors:",vectors_size,"%-15s" % "Bytes",vectors_size/1024,"kBytes","\t") 
print("%-20s" % "retention_reset:",retention_reset_size,"%-15s" % "Bytes",retention_reset_size/1024,"kBytes","\t")
print("%-20s" % "aes_data:",aes_data_size,"%-15s" % "Bytes",aes_data_size/1024,"kBytes","\t")
print("%-20s" % "retention_data:",retention_data_size,"%-15s" % "Bytes",retention_data_size/1024,"kBytes","\t")
print("%-20s" % "ram_code:",ram_code_size,"%-15s" % "Bytes",ram_code_size/1024,"kBytes","\t")
print("%-20s" % "exec_itable:",exec_itable_size,"%-15s" % "Bytes",exec_itable_size/1024,"kBytes","\t")
print("%-20s" % "iram_noinit_data:",iram_noinit_data_size,"%-15s" % "Bytes",iram_noinit_data_size/1024,"kBytes","\t")
if iram_bss_size > 0:
    print("%-20s" % "iram_bss:",iram_bss_size,"%-15s" % "Bytes",iram_bss_size/1024,"kBytes","\t")
print("%-20s" % "text:",text_size,"%-15s" % "Bytes",text_size/1024,"kBytes","\t")
print("%-20s" % "rodata:",rodata_size,"%-15s" % "Bytes",rodata_size/1024,"kBytes","\t")
print("%-20s" % "eh_frame:",eh_frame_size,"%-15s" % "Bytes",eh_frame_size/1024,"kBytes","\t")
print("%-20s" % "data:",data_size,"%-15s" % "Bytes",data_size/1024,"kBytes","\t")
print("%-20s" % "sbss:",sbss_size,"%-15s" % "Bytes",sbss_size/1024,"kBytes","\t")
print("%-20s" % "bss",bss_size,"%-15s" % "Bytes",bss_size/1024,"kBytes","\t")
print("%-20s" % "sdk_version",sdk_version_size,"%-15s" % "Bytes",sdk_version_size/1024,"kBytes","\t")


########## 统计IRAM大小 #########
IRAM_Size = retention_reset_size + aes_data_size + retention_data_size + ram_code_size + exec_itable_size + iram_bss_size + iram_noinit_data_size

########## 统计DRAM大小 #########
DRAM_Size = data_size + sbss_size + bss_size + sdk_version_size

########## 统计总RAM大小 #########
RAM_Size = IRAM_Size + DRAM_Size

########## 统计flash大小 #########
Flash_Size = vectors_size + retention_reset_size + aes_data_size + retention_data_size + ram_code_size + exec_itable_size + text_size + rodata_size + eh_frame_size + data_size + sdk_version_size

print("\t\n")
print("***************************calculate RAM size and Flash size*******************\t")
print("IRAM size:",IRAM_Size, "Bytes   ", IRAM_Size/1024, "kBytes","\t\nDRAM size:", DRAM_Size,"Bytes   ",DRAM_Size/1024, "kBytes","\t\nTotal RAM size:",RAM_Size,"Bytes   ",RAM_Size/1024, "kBytes",'\t\nFlash size:',Flash_Size,"Bytes   ",Flash_Size/1024, "kBytes\t\n")