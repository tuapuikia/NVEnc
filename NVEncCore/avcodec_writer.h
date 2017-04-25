﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------
#ifndef _AVCODEC_WRITER_H_
#define _AVCODEC_WRITER_H_

#include "NVEncVersion.h"

#if ENABLE_AVCUVID_READER
#include <thread>
#include <atomic>
#include <cstdint>
#include "avcodec_qsv.h"
#include "avcodec_reader.h"
#include "NVEncOutput.h"

using std::vector;

#define USE_CUSTOM_IO 1

#define USE_AVCODECPAR 1

static const int SUB_ENC_BUF_MAX_SIZE = 1024 * 1024;

typedef struct AVMuxFormat {
    const TCHAR          *pFilename;            //出力ファイル名
    AVFormatContext      *pFormatCtx;           //出力ファイルのformatContext
    char                  metadataStr[256];     //出力ファイルのエンコーダ名
    AVOutputFormat       *pOutputFmt;           //出力ファイルのoutputFormat

#if USE_CUSTOM_IO
    uint8_t              *pAVOutBuffer;         //avio_alloc_context用のバッファ
    uint32_t              nAVOutBufferSize;     //avio_alloc_context用のバッファサイズ
    FILE                 *fpOutput;             //出力ファイルポインタ
    char                 *pOutputBuffer;        //出力ファイルポインタ用のバッファ
    uint32_t              nOutputBufferSize;    //出力ファイルポインタ用のバッファサイズ
#endif //USE_CUSTOM_IO
    bool                  bStreamError;         //エラーが発生
    bool                  bIsMatroska;          //mkvかどうか
    bool                  bIsPipe;              //パイプ出力かどうか
    bool                  bFileHeaderWritten;   //ファイルヘッダを出力したかどうか
    AVDictionary         *pHeaderOptions;       //ヘッダオプション
} AVMuxFormat;

typedef struct AVMuxVideo {
    AVCodec              *pCodec;               //出力映像のCodec
    AVCodecContext       *pCodecCtx;            //出力映像のCodecContext
    AVRational            nFPS;                 //出力映像のフレームレート
    AVStream             *pStreamOut;           //出力ファイルの映像ストリーム
    bool                  bDtsUnavailable;      //出力映像のdtsが無効 (API v1.6以下)
    const AVStream       *pStreamIn;            //入力映像のストリーム
    int64_t               nInputFirstKeyPts;    //入力映像の最初のpts
    int                   nFpsBaseNextDts;      //出力映像のfpsベースでのdts (API v1.6以下でdtsが計算されない場合に使用する)
    bool                  bIsPAFF;              //出力映像がPAFFである
    int                   nBframeDelay;         //Bフレームによる遅延
    FILE                 *fpTsLogFile;          //mux timestampログファイル
} AVMuxVideo;

typedef struct AVMuxAudio {
    int                   nInTrackId;           //ソースファイルの入力トラック番号
    int                   nInSubStream;         //ソースファイルの入力サブストリーム番号
    const AVStream       *pStreamIn;            //入力音声のストリーム
    int                   nStreamIndexIn;       //入力音声のStreamのindex
    int                   nDelaySamplesOfAudio; //入力音声の遅延 (pkt_timebase基準)
    AVStream             *pStreamOut;           //出力ファイルの音声ストリーム
    int                   nPacketWritten;       //出力したパケットの数

    //変換用
    AVCodec              *pOutCodecDecode;      //変換する元のコーデック
    AVCodecContext       *pOutCodecDecodeCtx;   //変換する元のCodecContext
    AVCodec              *pOutCodecEncode;      //変換先の音声のコーデック
    AVCodecContext       *pOutCodecEncodeCtx;   //変換先の音声のCodecContext
    uint32_t              nIgnoreDecodeError;   //デコード時に連続して発生したエラー回数がこの閾値を以下なら無視し、無音に置き換える
    uint32_t              nDecodeError;         //デコード処理中に連続してエラーが発生した回数
    bool                  bEncodeError;         //エンコード処理中にエラーが発生

    //filter
    int                   nFilterInChannels;      //現在のchannel数      (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    uint64_t              nFilterInChannelLayout; //現在のchannel_layout (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    int                   nFilterInSampleRate;    //現在のsampling rate  (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    AVSampleFormat        FilterInSampleFmt;      //現在のSampleformat   (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    const TCHAR          *pFilter;
    AVFilterContext      *pFilterBufferSrcCtx;
    AVFilterContext      *pFilterBufferSinkCtx;
    AVFilterContext      *pFilterAudioFormat;
    AVFilterGraph        *pFilterGraph;

    //現在の音声のフォーマット
    int                   nResamplerInChannels;            //現在のchannel数      (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    uint64_t              nResamplerInChannelLayout;       //現在のchannel_layout (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    int                   nResamplerInSampleRate;          //現在のsampling rate  (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    AVSampleFormat        ResamplerInSampleFmt;            //現在のSampleformat   (pSwrContext == nullptrなら、encoderの入力、そうでないならresamplerの入力)
    
    //resampler
    int                   nAudioResampler;      //resamplerの選択 (QSV_RESAMPLER_xxx)
    SwrContext           *pSwrContext;          //Sampleformatの変換用
    uint8_t             **pSwrBuffer;           //Sampleformatの変換用のバッファ
    uint32_t              nSwrBufferSize;       //Sampleformatの変換用のバッファのサイズ
    int                   nSwrBufferLinesize;   //Sampleformatの変換用
    AVFrame              *pDecodedFrameCache;   //デコードされたデータのキャッシュされたもの
    int                   channelMapping[MAX_SPLIT_CHANNELS];        //resamplerで使用するチャンネル割り当て(入力チャンネルの選択)
    uint64_t              pnStreamChannelSelect[MAX_SPLIT_CHANNELS]; //入力音声の使用するチャンネル
    uint64_t              pnStreamChannelOut[MAX_SPLIT_CHANNELS];    //出力音声のチャンネル

    //AACの変換用
    AVBSFContext         *pAACBsfc;             //必要なら使用するbitstreamfilter
    int                   nAACBsfErrorFromStart; //開始直後からのbitstream filter errorの数

    int                   nOutputSamples;       //出力音声の出力済みsample数
    int64_t               nLastPtsIn;           //入力音声の前パケットのpts
    int64_t               nLastPtsOut;          //出力音声の前パケットのpts
} AVMuxAudio;

typedef struct AVMuxSub {
    int                   nInTrackId;           //ソースファイルの入力トラック番号
    const AVStream       *pStreamIn;            //入力字幕のストリーム
    int                   nStreamIndexIn;       //入力字幕のStreamのindex
    AVStream             *pStreamOut;           //出力ファイルの字幕ストリーム

    //変換用
    AVCodec              *pOutCodecDecode;      //変換する元のコーデック
    AVCodecContext       *pOutCodecDecodeCtx;   //変換する元のCodecContext
    AVCodec              *pOutCodecEncode;      //変換先の音声のコーデック
    AVCodecContext       *pOutCodecEncodeCtx;   //変換先の音声のCodecContext

    uint8_t              *pBuf;                 //変換用のバッファ
} AVMuxSub;

enum {
    MUX_DATA_TYPE_NONE   = 0,
    MUX_DATA_TYPE_PACKET = 1, //AVPktMuxDataに入っているデータがAVPacket
    MUX_DATA_TYPE_FRAME  = 2, //AVPktMuxDataに入っているデータがAVFrame
};

typedef struct AVPktMuxData {
    int         type;        //MUX_DATA_TYPE_xxx
    AVPacket    pkt;         //type == MUX_DATA_TYPE_PACKET 時有効
    AVMuxAudio *pMuxAudio;   //type == MUX_DATA_TYPE_PACKET 時有効
    int64_t     dts;         //type == MUX_DATA_TYPE_PACKET 時有効
    int         samples;     //type == MUX_DATA_TYPE_PACKET 時有効
    AVFrame    *pFrame;      //type == MUX_DATA_TYPE_FRAME 時有効
    int         got_result;  //type == MUX_DATA_TYPE_FRAME 時有効
} AVPktMuxData;

enum {
    AUD_QUEUE_PROCESS = 0,
    AUD_QUEUE_ENCODE  = 1,
    AUD_QUEUE_OUT     = 2,
};

#if ENABLE_AVCODEC_OUT_THREAD
typedef struct AVMuxThread {
    bool                         bEnableOutputThread;       //出力スレッドを使用する
    bool                         bEnableAudProcessThread;   //音声処理スレッドを使用する
    bool                         bEnableAudEncodeThread;    //音声エンコードスレッドを使用する
    std::atomic<bool>            bAbortOutput;              //出力スレッドに停止を通知する
    std::thread                  thOutput;                  //出力スレッド(mux部分を担当)
    std::atomic<bool>            bThAudProcessAbort;        //音声処理スレッドに停止を通知する
    std::thread                  thAudProcess;              //音声処理スレッド(デコード/thAudEncodeがなければエンコードも担当)
    std::atomic<bool>            bThAudEncodeAbort;         //音声エンコードスレッドに停止を通知する
    std::thread                  thAudEncode;               //音声エンコードスレッド(エンコードを担当)
    HANDLE                       heEventPktAddedOutput;     //キューのいずれかにデータが追加されたことを通知する
    HANDLE                       heEventClosingOutput;      //出力スレッドが停止処理を開始したことを通知する
    HANDLE                       heEventPktAddedAudProcess; //キューのいずれかにデータが追加されたことを通知する
    HANDLE                       heEventClosingAudProcess;  //音声処理スレッドが停止処理を開始したことを通知する
    HANDLE                       heEventPktAddedAudEncode;  //キューのいずれかにデータが追加されたことを通知する
    HANDLE                       heEventClosingAudEncode;   //音声処理スレッドが停止処理を開始したことを通知する
    CQueueSPSP<nvBitstream, 64>  qVideobitstreamFreeI;      //映像 Iフレーム用に空いているデータ領域を格納する
    CQueueSPSP<nvBitstream, 64>  qVideobitstreamFreePB;     //映像 P/Bフレーム用に空いているデータ領域を格納する
    CQueueSPSP<nvBitstream, 64>  qVideobitstream;           //映像パケットを出力スレッドに渡すためのキュー
    CQueueSPSP<AVPktMuxData, 64> qAudioPacketProcess;       //処理前音声パケットをデコード/エンコードスレッドに渡すためのキュー
    CQueueSPSP<AVPktMuxData, 64> qAudioFrameEncode;         //デコード済み音声フレームをエンコードスレッドに渡すためのキュー
    CQueueSPSP<AVPktMuxData, 64> qAudioPacketOut;           //音声パケットを出力スレッドに渡すためのキュー
} AVMuxThread;
#endif

typedef struct AVMux {
    AVMuxFormat         format;
    AVMuxVideo          video;
    vector<AVMuxAudio>  audio;
    vector<AVMuxSub>    sub;
    vector<sTrim>       trim;
#if ENABLE_AVCODEC_OUT_THREAD
    AVMuxThread         thread;
#endif
} AVMux;

typedef struct AVOutputStreamPrm {
    AVDemuxStream src;           //入力音声・字幕の情報
    const TCHAR  *pEncodeCodec;  //音声をエンコードするコーデック
    int           nBitrate;      //ビットレートの指定
    int           nSamplingRate; //サンプリング周波数の指定
    const TCHAR  *pFilter;       //音声フィルタ
} AVOutputStreamPrm;

struct AVOutputVideoPrm {
    GUID                         encCodecGUID;
    int                          nEncWidth;
    int                          nEncHeight;
    const NV_ENC_CONFIG         *pEncConfig;
    NV_ENC_PIC_STRUCT            nPicStruct;
    std::pair<int, int>          sar;
    AVRational                   outFps;
    NV_ENC_CONFIG_H264_VUI_PARAMETERS videoSignalInfo; //出力映像の情報
    bool                         bDtsUnavailable;      //出力映像のdtsが無効 (API v1.6以下)
    const AVStream              *pInputStream;         //入力映像のストリーム
    int64_t                      nInputFirstKeyPts;    //入力映像の最初のpts
};

struct AvcodecWriterPrm {
    const AVDictionary          *pInputFormatMetadata;    //入力ファイルのグローバルメタデータ
    const TCHAR                 *pOutputFormat;           //出力のフォーマット
    AVOutputVideoPrm             vidPrm;                  //出力映像の情報
    vector<sTrim>                trimList;                //Trimする動画フレームの領域のリスト
    vector<AVOutputStreamPrm>    inputStreamList;         //入力ファイルの音声・字幕の情報
    vector<const AVChapter *>    chapterList;             //チャプターリスト
    int                          nAudioResampler;         //音声のresamplerの選択
    uint32_t                     nAudioIgnoreDecodeError; //音声デコード時に発生したエラーを無視して、無音に置き換える
    int                          nBufSizeMB;              //出力バッファサイズ
    int                          nOutputThread;           //出力スレッド数
    int                          nAudioThread;            //音声処理スレッド数
    muxOptList                   vMuxOpt;                 //mux時に使用するオプション
    const TCHAR                 *pMuxVidTsLogFile;        //mux timestampログファイル

    AvcodecWriterPrm() :
        pInputFormatMetadata(nullptr),
        pOutputFormat(nullptr),
        vidPrm(),
        trimList(),
        inputStreamList(),
        nAudioResampler(0),
        nAudioIgnoreDecodeError(0),
        nBufSizeMB(0),
        nOutputThread(0),
        nAudioThread(0),
        vMuxOpt(),
        pMuxVidTsLogFile(nullptr) {
        memset(&vidPrm, 0, sizeof(vidPrm));
    }
};

class CAvcodecWriter : public NVEncOut
{
public:
    CAvcodecWriter();
    virtual ~CAvcodecWriter();

    virtual int Init(const TCHAR *strFileName, const void *option, shared_ptr<EncodeStatus> pEncSatusInfo) override;

    virtual int SetVideoParam(const NV_ENC_CONFIG *pEncConfig, NV_ENC_PIC_STRUCT pic_struct, const NV_ENC_SEQUENCE_PARAM_PAYLOAD *pSequenceParam) override;

    virtual int WriteNextFrame(const NV_ENC_LOCK_BITSTREAM *pNVEncBitstream) override;

    virtual int WriteNextFrame(uint8_t *ptr, uint32_t nSize) override;

    virtual int WriteNextPacket(AVPacket *pkt);

    virtual vector<int> GetStreamTrackIdList();

    virtual void Close() override;

#if USE_CUSTOM_IO
    int readPacket(uint8_t *buf, int buf_size);
    int writePacket(uint8_t *buf, int buf_size);
    int64_t seek(int64_t offset, int whence);
#endif //USE_CUSTOM_IO
    //出力スレッドのハンドルを取得する
    HANDLE getThreadHandleOutput();
    HANDLE getThreadHandleAudProcess();
    HANDLE getThreadHandleAudEncode();
private:
    //別のスレッドで実行する場合のスレッド関数 (出力)
    int WriteThreadFunc();

    //別のスレッドで実行する場合のスレッド関数 (音声処理)
    int ThreadFuncAudThread();

    //別のスレッドで実行する場合のスレッド関数 (音声エンコード処理)
    int ThreadFuncAudEncodeThread();

    //音声出力キューに追加 (音声処理スレッドが有効な場合のみ有効)
    int AddAudQueue(AVPktMuxData *pktData, int type);

    //AVPktMuxDataを初期化する
    AVPktMuxData pktMuxData(const AVPacket *pkt);

    //AVPktMuxDataを初期化する
    AVPktMuxData pktMuxData(AVFrame *pFrame);

    //WriteNextFrameの本体
    int WriteNextFrameInternal(nvBitstream *pBitstream, int64_t *pWrittenDts);

    //WriteNextPacketの本体
    int WriteNextPacketInternal(AVPktMuxData *pktData);

    //WriteNextPacketの音声処理部分(デコード/thAudEncodeがなければエンコードも担当)
    int WriteNextPacketAudio(AVPktMuxData *pktData);

    //WriteNextPacketの音声処理部分(エンコード)
    int WriteNextPacketAudioFrame(AVPktMuxData *pktData);

    //フィルタリング後のパケットをサブトラックに分配する
    int WriteNextPacketToAudioSubtracks(AVPktMuxData *pktData);

    //音声フレームをエンコード
    int WriteNextAudioFrame(AVPktMuxData *pktData);

    //音声のフィルタリングを実行
    int AudioFilterFrame(AVPktMuxData *pktData);

    //CodecIDがPCM系かどうか判定
    bool codecIDIsPCM(AVCodecID targetCodec);

    //PCMのコーデックがwav出力時に変換を必要とするかを判定する
    AVCodecID PCMRequiresConversion(const AVCodecParameters *pCodecParm);

    //NVEncのコーデックFourccからAVCodecのCodecIDを返す
    AVCodecID getAVCodecId(cudaVideoCodec cuvid_cc);

    //AAC音声にBitstreamフィルターを適用する
    int applyBitstreamFilterAAC(AVPacket *pkt, AVMuxAudio *pMuxAudio);

    //H.264ストリームからPAFFのフィールドの長さを返す
    uint32_t getH264PAFFFieldLength(uint8_t *ptr, uint32_t size, int *isIDR);

    //extradataをコピーする
    void SetExtraData(AVCodecContext *codecCtx, const uint8_t *data, uint32_t size);
#if USE_AVCODECPAR
    void SetExtraData(AVCodecParameters *pCodecParam, const uint8_t *data, uint32_t size);
#endif
    
    //映像の初期化
    int InitVideo(const AvcodecWriterPrm *prm);

    //音声フィルタの初期化
    int InitAudioFilter(AVMuxAudio *pMuxAudio, int channels, uint64_t channel_layout, int sample_rate, AVSampleFormat sample_fmt);

    //音声の初期化
    int InitAudioResampler(AVMuxAudio *pMuxAudio, int channels, uint64_t channel_layout, int sample_rate, AVSampleFormat sample_fmt);

    //音声の初期化
    int InitAudio(AVMuxAudio *pMuxAudio, AVOutputStreamPrm *pInputAudio, uint32_t nAudioIgnoreDecodeError);

    //字幕の初期化
    int InitSubtitle(AVMuxSub *pMuxSub, AVOutputStreamPrm *pInputSubtitle);

    //チャプターをコピー
    int SetChapters(const vector<const AVChapter *>& chapterList);

    //メッセージを作成
    tstring GetWriterMes();

    //対象のパケットの必要な対象のストリーム情報へのポインタ
    AVMuxAudio *getAudioPacketStreamData(const AVPacket *pkt);

    //対象のパケットの必要な対象のストリーム情報へのポインタ
    AVMuxAudio *getAudioStreamData(int nTrackId, int nSubStreamId = 0);

    //対象のパケットの必要な対象のストリーム情報へのポインタ
    AVMuxSub *getSubPacketStreamData(const AVPacket *pkt);

    //音声のchannel_layoutを自動選択する
    uint64_t AutoSelectChannelLayout(const uint64_t *pChannelLayout, const AVCodecContext *pSrcAudioCtx);

    //音声のsample formatを自動選択する
    AVSampleFormat AutoSelectSampleFmt(const AVSampleFormat *pSamplefmtList, const AVCodecContext *pSrcAudioCtx);

    //音声のサンプリングレートを自動選択する
    int AutoSelectSamplingRate(const int *pSamplingRateList, int nSrcSamplingRate);

    //音声ストリームをすべて吐き出す
    void AudioFlushStream(AVMuxAudio *pMuxAudio, int64_t *pWrittenDts);

    //音声をデコード
    AVFrame *AudioDecodePacket(AVMuxAudio *pMuxAudio, AVPacket *pkt);

    //音声をresample
    int AudioResampleFrame(AVMuxAudio *pMuxAudio, AVFrame **frame);

    //音声をエンコード
    vector<AVPktMuxData> AudioEncodeFrame(AVMuxAudio *pMuxAudio, const AVFrame *frame);

    //字幕パケットを書き出す
    int SubtitleTranscode(const AVMuxSub *pMuxSub, AVPacket *pkt);

    //字幕パケットを書き出す
    int SubtitleWritePacket(AVPacket *pkt);

    //パケットを実際に書き出す
    void WriteNextPacketProcessed(AVPktMuxData *pktData);

    //パケットを実際に書き出す
    void WriteNextPacketProcessed(AVPktMuxData *pktData, int64_t *pWrittenDts);

    //パケットを実際に書き出す
    void WriteNextPacketProcessed(AVMuxAudio *pMuxAudio, AVPacket *pkt, int samples, int64_t *pWrittenDts);

    //extradataに動画のヘッダーをセットする
    int SetSPSPPSToExtraData(const NV_ENC_SEQUENCE_PARAM_PAYLOAD *pSequenceParam);

    //extradataにHEVCのヘッダーを追加する
    int AddHEVCHeaderToExtraData(const nvBitstream *pBitstream);

    //ファイルヘッダーを書き出す
    int WriteFileHeader(const nvBitstream *pBitstream);

    //タイムスタンプをTrimなどを考慮しつつ計算しなおす
    //nTimeInがTrimで切り取られる領域の場合
    //lastValidFrame ... true 最後の有効なフレーム+1のtimestampを返す / false .. AV_NOPTS_VALUEを返す
    int64_t AdjustTimestampTrimmed(int64_t nTimeIn, AVRational timescaleIn, AVRational timescaleOut, bool lastValidFrame);

    void CloseSubtitle(AVMuxSub *pMuxSub);
    void CloseAudio(AVMuxAudio *pMuxAudio);
    void CloseVideo(AVMuxVideo *pMuxVideo);
    void CloseFormat(AVMuxFormat *pMuxFormat);
    void CloseThread();
    void CloseQueues();

    AVMux m_Mux;
    vector<AVPktMuxData> m_AudPktBufFileHead; //ファイルヘッダを書く前にやってきた音声パケットのバッファ
};

#endif //ENABLE_AVCUVID_READER

#endif //_AVCODEC_WRITER_H_
