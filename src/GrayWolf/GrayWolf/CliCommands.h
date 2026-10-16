#pragma once

#include "CliOptions.h"

int RunSelfTest(const CliOptions& Options);
int RunListInstalls(const CliOptions& Options);
int RunIdentify(const CliOptions& Options);
int RunDecompressFF(const CliOptions& Options);

int RunIndexAssets(const CliOptions& Options);
int RunQueryAssets(const CliOptions& Options);
int RunLoadFileMode(const CliOptions& Options);

int RunOatExtract(const CliOptions& Options);
int RunCoverage(const CliOptions& Options);
int RunOatFx(const CliOptions& Options);
int RunOatFxExport(const CliOptions& Options);
int RunOatXModelMtl(const CliOptions& Options);
int RunOatWeapon(const CliOptions& Options);
int RunOatRawfiles(const CliOptions& Options);
int RunActsExtract(const CliOptions& Options);
int RunUpdateNameDb(const CliOptions& Options);
int RunUpdate(const CliOptions& Options);
int RunApplyUpdate(const CliOptions& Options);
int RunImportHashCsv(const CliOptions& Options);

int RunBo3Images(const CliOptions& Options);
int RunBo3DumpXpakEntries(const CliOptions& Options);
int RunSalsa20SelfTest(const CliOptions& Options);
int RunBo3DecryptMovie(const CliOptions& Options);

int RunCascList(const CliOptions& Options);
int RunXpakIndex(const CliOptions& Options);
int RunXsubIndex(const CliOptions& Options);
int RunXsubExtract(const CliOptions& Options);
int RunCascExtractDir(const CliOptions& Options);
int RunZoneDecompress(const CliOptions& Options);
int RunBo4ExportLocalizeDisk(const CliOptions& Options);
int RunBo4ExportRawStringsDisk(const CliOptions& Options);
int RunBo4Decrypt(const CliOptions& Options);

int RunZoneWalk(const CliOptions& Options);

int RunBo4ZoneDiag(const CliOptions& Options, const std::string& Input);

int RunAudioProbe(const CliOptions& Options);
int RunDecodeIwd(const CliOptions& Options);
int RunVerifyOpus(const CliOptions& Options);
int RunSabProbe(const CliOptions& Options);

int RunGltfSelfTest(const CliOptions& Options);
int RunExportMapGlb(const CliOptions& Options);
int RunOpenMap(const CliOptions& Options);
int RunCerberusSelfTest(const CliOptions& Options);
int RunSeAnimSelfTest(const CliOptions& Options);
int RunCastSelfTest(const CliOptions& Options);
int RunMakeTestSeAnim(const CliOptions& Options);
int RunRenderAnimModel(const CliOptions& Options);
int RunXAnimBinDump(const CliOptions& Options);
int RunXAnimBinSelfTest(const CliOptions& Options);
int RunXAnimBinToSeAnimIn(const CliOptions& Options);
int RunXAnimBinExportDirIn(const CliOptions& Options);
int RunGenDisc(const CliOptions& Options);
int RunDumpModel(const CliOptions& Options);
int RunDumpGlb(const CliOptions& Options);
int RunRenderGlbPng(const CliOptions& Options);
int RunUiMode(const CliOptions& Options);
int RunPreviewWindow(const CliOptions& Options);
int RunRenderModelCli(const CliOptions& Options);

