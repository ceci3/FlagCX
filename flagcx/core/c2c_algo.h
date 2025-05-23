#ifndef FLAGCX_C2C_ALGO_H_
#define FLAGCX_C2C_ALGO_H_

#include "adaptor.h"
#include "collectives.h"
#include "flagcx.h"
#include "group.h"
#include "param.h"
#include <list>
#include <map>
#include <string>

struct flagcxBufferInfo {
public:
  flagcxBufferInfo(int offset, int count, int clusterIdToSend, int isRecv,
                   int isScheduled, int peerRank)
      : offset_(offset), count_(count), clusterIdToSend_(clusterIdToSend),
        isRecv_(isRecv), isScheduled_(isScheduled), peerRank_(peerRank) {}
  ~flagcxBufferInfo() {}

  int offset_;
  int count_;
  int clusterIdToSend_; // only required for send
  int isRecv_;          // 0: send, 1: recv
  int isScheduled_;     // 0: un-scheduled, 1: scheduled
  int peerRank_;
};

class flagcxInterRankBufferInfoManager {
public:
  flagcxInterRankBufferInfoManager(int totalCount);
  ~flagcxInterRankBufferInfoManager();

  bool checkIfPossibleToPush(int clusterId, int rank, int offset, int count);
  bool checkIfPossibleToSplitAndPush(int clusterId, int rank, int offset,
                                     int count, int *splitCount, int *pushMode);
  bool checkIsFull(int clusterId, int rank);
  bool checkIsScheduled(int clusterId, int rank);
  std::list<flagcxBufferInfo> &getBufferInfoList(int clusterId, int rank);
  void pushBackBufferInfo(int clusterId, int rank, int offset, int count,
                          int clusterIdToSend, int isRecv, int isScheduled,
                          int peerRank);
  void popFrontBufferInfo(int clusterId, int rank);
  void resetBufferInfo();
  void printBufferInfo(int step); // 0: intial, 1: internal, 2: final

  int totalCount_; // total communication count
  std::map<int, std::map<int, std::list<flagcxBufferInfo>>>
      bufferInfos_; // map<clusterId, map<rank, list[struct{offset, count,
                    // isRecv, isScheduled}]>>
};

class flagcxC2cP2pOp {
public:
  flagcxC2cP2pOp(int rank, int offset, int peerRank, int peerOffset, int count,
                 int isRecv);
  ~flagcxC2cP2pOp();

  flagcxResult_t run(const void *sendbuff, void *recvbuff,
                     flagcxDataType_t datatype, flagcxComm_t comm,
                     flagcxStream_t stream);

  int rank_;
  int offset_;
  int peerRank_;
  int peerOffset_;
  int count_;
  int isRecv_; // 0: send, 1: recv
};

class flagcxC2cHomoFunc {
public:
  flagcxC2cHomoFunc(int rank, int rootRank, int offset, int count,
                    flagcxCommOp_t commOp);
  ~flagcxC2cHomoFunc();

  flagcxResult_t run(const void *sendbuff, void *recvbuff,
                     flagcxDataType_t datatype, int root, flagcxComm_t comm,
                     flagcxStream_t stream);

  int rank_;
  int rootRank_;
  int offset_;
  int count_;
  flagcxCommOp_t commOp_;
};

class flagcxC2cHeteroFunc {
public:
  flagcxC2cHeteroFunc();
  ~flagcxC2cHeteroFunc();

  void addP2pOp(int rank, int offset, int peerRank, int peerOffset, int count,
                int isRecv);
  flagcxResult_t run(const void *sendbuff, void *recvbuff,
                     flagcxDataType_t datatype, flagcxComm_t comm,
                     flagcxStream_t stream);

private:
  std::vector<flagcxC2cP2pOp> p2pOps_;
};

class flagcxC2cPlanner {
public:
  flagcxC2cPlanner(int clusterId, int rank, int homoMyRank, int homoRootRank,
                   int homoRanks, int homoInterMyRank, int homoInterRootRank,
                   int homoInterRanks, int totalCount, flagcxCommOp_t commOp,
                   flagcxRedOp_t redOp,
                   std::vector<std::vector<int>> &clusterInterRankList);
  ~flagcxC2cPlanner();

  flagcxResult_t refresh(
      int isSendRecv); // 0: refresh recv info only; 1: refresh send+recv info
  flagcxResult_t findStrategy();
  flagcxResult_t execute(const void *sendbuff, void *recvbuff,
                         flagcxDataType_t datatype, int root, flagcxComm_t comm,
                         flagcxStream_t stream);

private:
  int clusterId_;
  int rank_; // global rank
  int homoMyRank_;
  int homoRootRank_;
  int homoRanks_;
  int homoInterMyRank_;
  int homoInterRootRank_;
  int homoInterRanks_;
  int totalCount_;
  flagcxCommOp_t commOp_;
  flagcxRedOp_t redOp_;
  std::vector<std::vector<int>> &clusterInterRankList_;
  flagcxInterRankBufferInfoManager interRankBufferInfoManager_;
  int multiNic_;
  int eachNicPerRank_;
  int preHomoFuncLoops_;           // number of loops for preHomoFunc
  int heteroAndPostHomoFuncLoops_; // number of loops for heteroFunc and
                                   // postHomoFunc
  std::vector<flagcxC2cHomoFunc> preHomoFuncList_;
  std::vector<flagcxC2cHeteroFunc> heteroFuncList_;
  std::vector<flagcxC2cHomoFunc> postHomoFuncList_;
};

#endif // end include guard