# TypeScript 原生编译器的来源

- 项目：TypeScript（微软），https://github.com/microsoft/typescript-go
- 包：npm 的 `@typescript/typescript-darwin-arm64`，版本 7.0.2（`typescript@7.0.2` 的 macOS arm64 原生编译器，带微软的签名，Team ID `UBF8T346G9`；`typescript` 包里的 `tsc` 只是启动它的 Node 脚本）
- 来源包：https://registry.npmjs.org/@typescript/typescript-darwin-arm64/-/typescript-darwin-arm64-7.0.2.tgz，9 270 373 字节，SHA-256 `902e2fe1cf0799198ef902c6b8c310a450fef629a6baba41d45641ef75c04ebd`，npm 登记的完整性 `sha512-gowzar9MwS/aRWp6f3a4KUqzRjAZjOsmGNCM6LcTgXum+dBfgsBVMN+AgvOCCbguXyick6LJhpBszxMebJ8syA==`（下载后已核对一致）
- 2026-11-24 起只做 macOS：此前的 Windows 版 `tsc.exe`（`@typescript/typescript-win32-x64` 7.0.2）已换成这一份；标准库类型声明与两份包逐字节相同，`LICENSE`、`NOTICE.txt` 只差换行符。Universal 版还需 `@typescript/typescript-darwin-x64` 7.0.2，用 `lipo` 与本份合成一个通用 `tsc`
- 许可：Apache-2.0；`LICENSE` 与 `NOTICE.txt`（它引用的第三方声明）取自来源包，原样未改
- 取得日期：2026-10-31
- 用途：脚本的类型检查（ADR-0065"修订（2026-10-31，用户）"：程序自带原生编译器，写脚本的人不用装 Node）；由 `gs check` 与脚本工作台调用，程序运行脚本不用它（运行时只用内置擦除器）
- 保留：来源包的 `LICENSE`、`NOTICE.txt` 与 `lib/` 整个目录（`tsc` 与 108 个标准库类型声明 `lib.*.d.ts`，编译器在自己旁边找它们），原样未改；不带 `package.json`、`README.md`
- 构建：`src/cli/CMakeLists.txt` 把 `lib/` 复制到 `gs` 旁边的 `typescript/`；许可证由 `tools/gen_licenses.py` 收进"关于"页
- 升级：`npm pack @typescript/typescript-darwin-arm64@<版本>` 下载，核对 `npm view @typescript/typescript-darwin-arm64@<版本> dist.integrity`，解开后用包里的 `LICENSE`、`NOTICE.txt`、`lib/` 整个替换本目录对应内容，更新本文件与 `third_party/README.md` 的版本和校验和，重新跑 `gs_cli` 的检查测试

| 文件 | SHA256 |
|---|---|
| `LICENSE` | `a7d00bfd54525bc694b6e32f64c7ebcf5e6b7ae3657be5cc12767bce74654a47` |
| `NOTICE.txt` | `f5c708b59114507b8b27b48181b6883d106bbca0c1634bbee45b5e344237b66b` |
| `lib/tsc` | `a82f731365ad69d5c4c15f5e18fba4584bf3b7b839960172a76c3462b5114bf2` |
| `lib/lib.*.d.ts`（108 个） | 与来源包逐字节相同，按上面的来源包校验和重新解出即可核对 |
