LE Audio SDK注意事项

(1)打开SDK_RELEASE_CHECK_EN，确保相关宏全部关闭
(2)根据各个模块控制结构体长度，将所有sizeof换成数值。包括：   ADV_SET_PARAM_LENGTH
                                                         PERD_ADV_PARAM_LENGTH
                                                         BIG_BCST_PARAM_LENGTH
                                                         BIG_SYNC_PARAM_LENGTH
                                                         BIS_PARAM_LENGTH
                                                         CIS_SLV_PARAM_LEN
                                                         CIG_PARAM_LEN
                                                         CIS_CONN_PARAM_LENGTH
(3)检查le_audio_lib_gen.sh，Source Code如果库相关源文件有增删，需要对应调整。
(4)为防止反汇编风险，出库时Project Setting里面debug level应该设置为none。同时删除Miscellaneous选项里面的-fno-fat-lto-objects
   原因：Debug Level 主要是给Jlink使用的，在出库的时候，不希望外部用户能够看到这些debug信息，具有反翻译风险，所以出库的时候，改为None。
         B92 使用GCC10，内部使用时候默认开debug信息，这就导致lst文件特别大（GCC7没有这个问题），解决办法是加-fno-fat-lto-objects。
         但经过测试，当Debug Level=none的时候，应该去掉这个-fno-...。
   总结：出库时，无论是B91或B92或者是其他的芯片，都需要把Debug Level设为None.
         Debug level是None时，-fno-fat-lto-objects是无效的，如果Miscellaneous选项有设置，需要删除。
		  
(5)出库时必须关闭VCD_EN，否则VCD log会被编译进lib里面，影响audio功能
		 
该规范仅针对LE Audio profile开发代码。

1. 函数命名规范
[start]_[service][Y]_[one][Two][Three]
start: 需要给用户层调用的函数，使用blc。不开放给用户层的函数，使用blt。
service：函数所在的service，全部使用小写，如PACS、ASCS、MCS、TBS、GTBS、CSIS、MICS、VCS、VOCS、AICS、CAS、HAS、PBS、TMAS、VCP。以及通用层audio.
Y: s表示函数属于server，c表示函数属于client。
one/Two/Three: 表示函数实现的功能，如单个单词表示不了，使用驼峰原则，第二个单次开始首字母大写。

如ASCS server发送notify数据   blc_ascss_pushNtf
ASCS client接收notify数据     blc_ascsc_recvNtf

2. 变量命名规范
变量命名采用驼峰原则

3. 枚举数据命名规范
枚举命名全部采用大写，每个单词用_分割开。
枚举别名全部采用小写，每个单次用_分割开，并且以enum结尾，和结构体区分。
如LE Audio的角色枚举
typedef enum {
    AUDIO_CLIENT    = 0x00,
    AUDIO_SERVER    = 0x01,
} blc_audio_role_enum;

4. 结构体命名规范
[start]_[service][Y]_[one]_[two]_[three]_t
start: 需要给用户层调用的函数，使用blc。不开放给用户层的函数，使用blt。
service：函数所在的service，全部使用小写，如PACS、ASCS、MCS、TBS、GTBS、CSIS、MICS、VCS、VOCS、AICS、CAS、HAS、PBS、TMAS、VCP。以及通用层audio.
Y: s表示函数属于server，s表示函数属于client。
one/Two/Three: 表示函数实现的功能，如单个单词表示不了，使用_分割开。
t: 结尾表示是结构体
如MICS client的

5. LE Audio profile通用结构体和实体命名规范
client/server的数据结构体命名blc_[profile_name]_[client/server]_t
如ascs client命令为blc_ascs_client_t
ascs server命名为blc_ascs_server_t

client/server的控制结构体(携带audio process模块)命名blc_[profile_name]_[client/server]_ctrl_t
其中的client/server数据块命名为p[profile_name][Cient/Server]，如果是实体去掉'p',p是pointer简写。
如ascs client命令为blc_ascs_client_ctrl_t
typedef struct blc_ascs_client_ctrl {
    blc_audio_proc_t process;
    blc_ascs_client_t* pAscsClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_ascs_client_ctrl_t;
ascs server命名为blc_ascs_server_ctrl_t
typedef struct blc_ascs_server_ctrl{
	blc_audio_proc_t process;
	blc_ascs_server_t* pAscsServer[STACK_PRF_ACL_PERIPHRAL_MAX_NUM];
} blc_ascs_server_ctrl;

client/server的控制实例命名为 [profile_name]_[client/server]_ctrl

client/server初始化注册的结构体blc_[profile_name][c/s]_regParam_t

6. 缩写对照表
有一些英文单次需要缩写的，需要对照下表，如果没有的添加后方可使用，不然使用全拼。按照首字母排序
缩写            全拼
cap             capabilities
cback           callback function
cmd             command
cnt             count
cfm             confirmation
ctrl            controller / control
hdl             handle
ind             indication
info            information
ntf             Notification
src             source
spec            specific
req             request
rsp             response
supp            supported / support
vol             volume

content         content control
render_cap      rendering and capture control
stream          stream control
trans_coord     transition and coordination control
user_case       use case specific profiles

aicsc           audio input control service client
aicss           audio input control service server
ascsc           audio stream control service client
ascss           audio stream control service server
bassc           broadcast audio scan service client
basss           broadcast audio scan service server
casc            common audio service client
cass            common audio service server
csisc           coordinated set identification service client
csiss           coordinated set identification service server
hasc            hearing access service client
hass            hearing access service server
mcsc            media control service client
mcss            media control service server
micsc           microphone control service client
micss           microphone control service server
pacsc           published audio capabilities service client
pacss           published audio capabilities service server
pbsc            public broadcast service client
pbss            public broadcast service server
tbsc            telephone bearer service client
tbss            telephone bearer service server
tmasc           telephony and media audio service client
tmass           telephony and media audio service server
vcsc            volume control service client
vcss            volume control service server
vocsc           volume offset control service client
vocss           volume offset control service server


********************************************

LE Audio USB PID分配，VID固定0x248a

Unicast:       0x6000开始

(1)0x6001     上行16k双声道，下行48k双声道     
(2)0x6002     上行16k单声道，下行48k双声道
(3)0x6003     无上行，下行48k双声道


Broadcast:     0x6100开始
(1)0x6101       无上行，下行48k双声道
(2)0x6102       USB-CDC功能，目前Assistant使用
(3)0x6103       无上行，下行24K双声道
(4)0x6104       无上行，下行24K单声道

Broadcast Source USB-Speaker+USB-CDC功能
(5)0x6110       无上行，下行8K单声道，USB-CDC
(6)0x6111       无上行，下行8K双声道，USB-CDC
(5)0x6112       无上行，下行16K单声道，USB-CDC
(6)0x6113       无上行，下行16K双声道，USB-CDC
(5)0x6114       无上行，下行24K单声道，USB-CDC
(6)0x6115       无上行，下行24K双声道，USB-CDC
(5)0x6116       无上行，下行32K单声道，USB-CDC
(6)0x6117       无上行，下行32K双声道，USB-CDC
(5)0x6118       无上行，下行48K单声道，USB-CDC
(6)0x6119       无上行，下行48K双声道，USB-CDC
