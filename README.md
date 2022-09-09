# 升级包热安装部件

#### 简介
升级包热安装组件是在设备正常使用的情况下，在后台完成升级包的静默升级，其功能主要包括对升级包进行校验，确保升级包合法有效；然后从升级包中解析出升级的可执行程序，创建子进程并启动升级程序。具体升级的动作由升级脚本控制。

#### 目录

```
base/update/sys_installer/
├── frameworks                  # 热安装sa服务框架目录
│   ├── installer_manager       # 安装管理目录
│   ├── ipc_server              # sa服务端框架
│   └── status_manager          # 安装状态管理目录
├── interfaces
│   ├── inner_api               #对外api接口定义
│   └── innerkits               #对外api接口实现
└── services                    #热安装业务实现
```

