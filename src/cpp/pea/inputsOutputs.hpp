
#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "trace.hpp"

struct ReceiverMap;
struct Network;
struct KFState;
struct GTime;

void reloadInputFiles();

bool checkValidFile(
	const string&				path,
	const string&				description = "");

bool checkValidFiles(
	vector<string>&				paths,
	const string&				description = "");

void replaceTimes(
	string&						str,
	boost::posix_time::ptime	time);

void createDirectories(
	boost::posix_time::ptime	time);

void perEpochPostProcessingAndOutputs(
	Trace&			pppTrace,
	GTime			time,
	Network&		ionNet,
	ReceiverMap&	receiverMap,
	KFState&		kfState,
	bool			emptyEpoch,
	bool			inRts = false,
	bool			firstEpoch = false);

void createTracefiles(
	ReceiverMap&	receiverMap,
	Network&		pppNet,
	Network&		ionNet);

void outputPredictedStates(
	Trace&			trace,
	KFState&		kfState);

void configureUploadingStreams();
