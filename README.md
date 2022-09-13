# 升级包热安装部件

## 简介
升级包热安装组件是在设备正常使用的情况下，在后台完成升级包的静默升级，其功能主要是针对AB系统的设备提供一种热升级的能力，支持标准系统设备。

## 目录

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

## 相关仓

[升级子系统](https://gitee.com/openharmony/update_updater/blob/master/README_zh.md)

