/**
 * GLib - General C++ Library
 * 
 * Copyright (C) 2014 Jozef Stefan Institute
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

//////////////////////////////////////
// Simple-App-Server-Request-Environment
void TSAppSrvRqEnv::ExecFun(const TStr& FunNm, const TStrKdV& FldNmValPrV) { 
	EAssert(FunNmToFunH.IsKey(FunNm));
	FunNmToFunH.GetDat(FunNm)->Exec(FldNmValPrV, this); 
}

//////////////////////////////////////
// Simple-App-Server-Function
bool TSAppSrvFun::IsFldNm(const TStrKdV& FldNmValPrV, const TStr& FldNm) {
	const int ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""));
	return (ValN != -1);
}

TStr TSAppSrvFun::GetFldVal(const TStrKdV& FldNmValPrV, 
		const TStr& FldNm, const TStr& DefFldVal) {

	const int ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""));
	return (ValN == -1) ? DefFldVal : FldNmValPrV[ValN].Dat;	
}

int TSAppSrvFun::GetFldInt(const TStrKdV& FldNmValPrV, const TStr& FldNm) {
	EAssertR(IsFldNm(FldNmValPrV, FldNm), "Missing parameter '" + FldNm + "'");
	TStr IntStr = GetFldVal(FldNmValPrV, FldNm, "");
	EAssertR(IntStr.IsInt(), "Parameter '" + FldNm + "' not a number");
	return IntStr.GetInt();
}

int TSAppSrvFun::GetFldInt(const TStrKdV& FldNmValPrV, const TStr& FldNm, const int& DefInt) {
	if (!IsFldNm(FldNmValPrV, FldNm)) { return DefInt; }
	TStr IntStr = GetFldVal(FldNmValPrV, FldNm, "");
	EAssertR(IntStr.IsInt(), "Parameter '" + FldNm + "' not a number");
	return IntStr.GetInt();
}

bool TSAppSrvFun::GetFldBool(const TStrKdV& FldNmValPrV, const TStr& FldNm, const bool& DefVal)
{
	if (!IsFldNm(FldNmValPrV, FldNm)) return DefVal;
	TStr Val = GetFldVal(FldNmValPrV, FldNm, "").GetLc();
	if (Val == "1" || Val == "true") return true;
	return false;
}

uint64 TSAppSrvFun::GetFldUInt64(const TStrKdV& FldNmValPrV, const TStr& FldNm, const uint64& DefVal) {
	TStr IntStr = GetFldVal(FldNmValPrV, FldNm, TUInt64(DefVal).GetStr());
	EAssertR(IntStr.IsUInt64(), "Parameter '" + FldNm + "' not a number");
	return IntStr.GetUInt64();
}

void TSAppSrvFun::GetFldValV(const TStrKdV& FldNmValPrV, const TStr& FldNm, TStrV& FldValV) {
	FldValV.Clr();
	int ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""));
	while (ValN != -1) {
		FldValV.Add(FldNmValPrV[ValN].Dat);
		ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""), ValN + 1);
	}
}

void TSAppSrvFun::GetFldValSet(const TStrKdV& FldNmValPrV, const TStr& FldNm, TStrSet& FldValSet) {
	FldValSet.Clr();
	int ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""));
	while (ValN != -1) {
		FldValSet.AddKey(FldNmValPrV[ValN].Dat);
		ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""), ValN + 1);
	}
}

bool TSAppSrvFun::IsFldNmVal(const TStrKdV& FldNmValPrV, 
		const TStr& FldNm, const TStr& FldVal) {

	int ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""));
	while (ValN != -1) {
		if (FldNmValPrV[ValN].Dat == FldVal) { return true; }
		ValN = FldNmValPrV.SearchForw(TStrKd(FldNm, ""), ValN + 1);
	}
	return false;
}

TStr TSAppSrvFun::XmlHdStr = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";

PXmlDoc TSAppSrvFun::GetErrorXmlRes(const TStr& ErrorStr) {
	return TXmlDoc::New(TXmlTok::New("error", ErrorStr)); 
}

TStr TSAppSrvFun::GetErrorJsonRes(const TStr& ErrorStr) {
	PJsonVal ErrorVal = TJsonVal::NewObj("error", TJsonVal::NewStr(ErrorStr));
	return TJsonVal::GetStrFromVal(ErrorVal);
}

void TSAppSrvFun::Exec(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
	const PNotify& Notify = RqEnv->GetWebSrv()->GetNotify();
	PHttpResp HttpResp;
	try {
        // log the call
		if (NotifyOnRequest)
			Notify->OnStatus(TStr::Fmt("[%s] RequestStart  %s", TTm::GetCurLocTm().GetWebLogDateTimeStr(true, " ", false).CStr(), FunNm.CStr()));
		TTmStopWatch StopWatch(true);
		// execute the actual function, according to the type
		PSIn BodySIn; TStr ContTypeVal;
		if (GetFunOutType() == saotXml) {
			PXmlDoc ResXmlDoc = ExecXml(FldNmValPrV, RqEnv);        
			TStr ResXmlStr; ResXmlDoc->SaveStr(ResXmlStr);
			BodySIn = TMIn::New(XmlHdStr + ResXmlStr);
			ContTypeVal = THttp::TextXmlFldVal;
		} else if (GetFunOutType() == saotJSon) {
			TStr ResStr = ExecJSon(FldNmValPrV, RqEnv);
			BodySIn = TMIn::New(ResStr);
			ContTypeVal = THttp::AppJSonFldVal;
		} else {
			BodySIn = ExecSIn(FldNmValPrV, RqEnv, ContTypeVal);
		}
		if (ReportResponseSize)
			Notify->OnStatusFmt("[%s] Response size: %.1f KB", TTm::GetCurLocTm().GetWebLogDateTimeStr(true, " ", false).CStr(), BodySIn->Len() / (double) TInt::Kilo);
		// log finish of the call
		if (NotifyOnRequest)
			Notify->OnStatus(TStr::Fmt("[%s] RequestFinish %s [request took %d ms]", TTm::GetCurLocTm().GetWebLogDateTimeStr(true, " ", false).CStr(), FunNm.CStr(), StopWatch.GetMSecInt()));
		// prepare response
		HttpResp = THttpResp::New(THttp::OkStatusCd, 
			ContTypeVal, false, BodySIn);
    } catch (PExcept Except) {
		// known internal error
        // known internal error
        Notify->OnStatusFmt("[%s] Exception: %s", TTm::GetCurLocTm().GetWebLogDateTimeStr(true, " ", false).CStr(), Except->GetMsgStr().CStr());
        Notify->OnStatusFmt("Location: %s", Except->GetLocStr().CStr());
        TStr ResStr, ContTypeVal = THttp::TextPlainFldVal;
		if (GetFunOutType() == saotXml) {
			PXmlTok TopTok = TXmlTok::New("error");
			TopTok->AddSubTok(TXmlTok::New("message", Except->GetMsgStr()));
			TopTok->AddSubTok(TXmlTok::New("location", Except->GetLocStr()));
			PXmlDoc ErrorXmlDoc = TXmlDoc::New(TopTok); 
			ResStr = XmlHdStr + ErrorXmlDoc->SaveStr();
            ContTypeVal = THttp::TextXmlFldVal;
		} else if (GetFunOutType() == saotJSon) {
			PJsonVal ResVal = TJsonVal::NewObj();
			ResVal->AddToObj("message", Except->GetMsgStr());
			ResVal->AddToObj("location", Except->GetLocStr());
			ResStr = TJsonVal::NewObj("error", ResVal)->SaveStr();
            ContTypeVal = THttp::AppJSonFldVal;
		}
        // prepare response
        HttpResp = THttpResp::New(THttp::InternalErrStatusCd, 
            ContTypeVal, false, TMIn::New(ResStr));        
    } catch (...) {
		// unknown internal error
		TStr ResStr, ContTypeVal = THttp::TextPlainFldVal;
		if (GetFunOutType() == saotXml) {
			PXmlDoc ErrorXmlDoc = TXmlDoc::New(TXmlTok::New("error")); 
			ResStr = XmlHdStr + ErrorXmlDoc->SaveStr();
            ContTypeVal = THttp::TextXmlFldVal;            
		} else if (GetFunOutType() == saotJSon) {
			ResStr = TJsonVal::NewObj("error", "Unknown")->SaveStr();
            ContTypeVal = THttp::AppJSonFldVal;
		}
		// prepare response
        HttpResp = THttpResp::New(THttp::InternalErrStatusCd, 
            ContTypeVal, false, TMIn::New(ResStr));
    }

	if (LogRqToFile)
		LogReqRes(FldNmValPrV, HttpResp);
	// send response
	RqEnv->GetWebSrv()->SendHttpResp(RqEnv->GetSockId(), HttpResp); 
}

void TSAppSrvFun::LogReqRes(const TStrKdV& FldNmValPrV, const PHttpResp& HttpResp)
{
	if (LogRqFolder == "")
		return;
	try	 {
		TDir::GenDir(LogRqFolder);
		TStr TimeNow = TTm::GetCurLocTm().GetWebLogDateTimeStr(true);
		TimeNow.ChangeChAll(':', '.');
		PSOut Output = TFOut::New(LogRqFolder + "/" + TimeNow + ".txt", false);
		Output->PutStr(FunNm.CStr()); Output->PutCh('\n');
		for (int N=0; N < FldNmValPrV.Len(); N++)
			Output->PutStrFmt("  %s: %s\n", FldNmValPrV[N].Key.CStr(), FldNmValPrV[N].Dat.CStr());
		Output->PutCh('\n');
		Output->PutStr(HttpResp->GetBodyAsStr(), false);
	} catch (...) {
		/*const PNotify& Notify = RqEnv->GetWebSrv()->GetNotify();
		Notify->OnStatus("Unable to log request for function '" + GetFunNm() + "'!");*/
	}
}

//////////////////////////////////////////////////////////////////////////
// Simple-App-Server
#include "favicon.cpp"

TSAppSrv::TSAppSrv(const int& PortN, const TSAppSrvFunV& SrvFunV, const PNotify& Notify, 
		const bool& _ShowParamP, const bool& _ListFunP): TWebSrv(PortN, true, Notify), 
		Favicon(Favicon_bf, Favicon_len) {

    ShowParamP = _ShowParamP;
    ListFunP = _ListFunP;
    // initiaize hash-table with mappings
    for (int SrvFunN = 0; SrvFunN < SrvFunV.Len(); SrvFunN++) {
        PSAppSrvFun SrvFun =  SrvFunV[SrvFunN];
        FunNmToFunH.AddDat(SrvFun->GetFunNm(), SrvFun);
    }
}

void TSAppSrv::OnHttpRq(const uint64& SockId, const PHttpRq& HttpRq) {
	// last appropriate error code, start with bad request
	int ErrStatusCd = THttp::BadRqStatusCd;
    try {
        // check http-request correctness - return if error
        EAssertR(HttpRq->IsOk(), "Bad HTTP request!");
        // check url correctness - return if error
        PUrl RqUrl = HttpRq->GetUrl();
        EAssertR(RqUrl->IsOk(), "Bad request URL!");
        // extract function name
        PUrl HttpRqUrl = HttpRq->GetUrl();
		TStr FunNm = HttpRqUrl->GetPathSeg(0);
		// check if we have the function registered
		if (FunNm == "favicon.ico") {
			PHttpResp HttpResp = THttpResp::New(THttp::OkStatusCd,
				THttp::ImageIcoFldVal, false, Favicon.GetSIn());
			SendHttpResp(SockId, HttpResp); 
			return;
		} else if (!FunNm.Empty() && !FunNmToFunH.IsKey(FunNm)) { 
			ErrStatusCd = THttp::ErrNotFoundStatusCd;
			GetNotify()->OnStatusFmt("[AppSrv] Unknown function '%s'!", FunNm.CStr());
			TExcept::Throw("Unknown function '" + FunNm + "'!");
		}
        // extract parameters
        TStrKdV FldNmValPrV;
        PUrlEnv HttpRqUrlEnv = HttpRq->GetUrlEnv();
        const int Keys = HttpRqUrlEnv->GetKeys();
        for (int KeyN = 0; KeyN < Keys; KeyN++) {
            TStr KeyNm = HttpRqUrlEnv->GetKeyNm(KeyN);
            const int Vals = HttpRqUrlEnv->GetVals(KeyN);
            for (int ValN = 0; ValN < Vals; ValN++) {
                TStr Val = HttpRqUrlEnv->GetVal(KeyN, ValN);
                FldNmValPrV.Add(TStrKd(KeyNm, Val));
            }
        }
		// report call
		if (ShowParamP) {  GetNotify()->OnStatus(" " + HttpRq->GetUrl()->GetUrlStr()); }
		// request parsed well, from now on it's internal error
		ErrStatusCd = THttp::InternalErrStatusCd;
		// processed requested function
		if (!FunNm.Empty()) {
			// prepare request environment
			PSAppSrvRqEnv RqEnv = TSAppSrvRqEnv::New(this, SockId, HttpRq, FunNmToFunH);
			// retrieve function
			PSAppSrvFun SrvFun = FunNmToFunH.GetDat(FunNm);
			// call function
			SrvFun->Exec(FldNmValPrV, RqEnv);
		} else {
			// internal SAppSrv call
			if (!ListFunP) {
				// we are not allowed to list functions
				ErrStatusCd = THttp::ErrNotFoundStatusCd;
				TExcept::Throw("Unknown page");
			}
			// prepare a list of registered functions
            PJsonVal FunArrVal = TJsonVal::NewArr();
			int KeyId = FunNmToFunH.FFirstKeyId();
			while (FunNmToFunH.FNextKeyId(KeyId)) {
                FunArrVal->AddToArr(TJsonVal::NewObj("name", FunNmToFunH.GetKey(KeyId)));
			}
            PJsonVal ResVal = TJsonVal::NewObj();
            ResVal->AddToObj("port", GetPortN());
            ResVal->AddToObj("connections", GetConns());            
            ResVal->AddToObj("functions", FunArrVal);
			TStr ResStr = ResVal->SaveStr();
			// prepare response
			PHttpResp HttpResp = THttpResp::New(THttp::OkStatusCd, 
				THttp::AppJSonFldVal, false, TMIn::New(ResStr));
			// send response
			SendHttpResp(SockId, HttpResp); 
		}
    } catch (PExcept Except) {
		// known internal error
        PJsonVal ErrorVal = TJsonVal::NewObj();
        ErrorVal->AddToObj("message", Except->GetMsgStr());
        ErrorVal->AddToObj("location", Except->GetLocStr());
        PJsonVal ResVal = TJsonVal::NewObj("error", ErrorVal);
        TStr ResStr = ResVal->SaveStr();
        // prepare response
		PHttpResp HttpResp = THttpResp::New(ErrStatusCd,
            THttp::AppJSonFldVal, false, TMIn::New(ResStr));
        // send response
	    SendHttpResp(SockId, HttpResp);
    } catch (...) {
		// unknown internal error
        PJsonVal ResVal = TJsonVal::NewObj("error", "Unknown internal error");
        TStr ResStr = ResVal->SaveStr();
        // prepare response
		PHttpResp HttpResp = THttpResp::New(ErrStatusCd,
            THttp::AppJSonFldVal, false, TMIn::New(ResStr));
        // send response
	    SendHttpResp(SockId, HttpResp);
    }
}

//////////////////////////////////////
// File-Download-Function
PSIn TSASFunFPath::ExecSIn(const TStrKdV& FldNmValPrV, 
        const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr) {

	// construct file name
    TStr FNm = FPath;
    PUrl Url = RqEnv->GetHttpRq()->GetUrl();
    const int PathSegs = Url->GetPathSegs();
    if ((PathSegs == 1) || (PathSegs == 2 && Url->GetPathSeg(1).Empty())) {
        // nothing specified, do the default
        TStr PathSeg = Url->GetPathSeg(0);
        if (PathSeg.LastCh() != '/') { FNm += "/"; }
        FNm += DefaultFNm; 
    } else {
        // extract file name
        for (int PathSegN = 1; PathSegN < PathSegs; PathSegN++) {
            FNm += "/"; FNm += Url->GetPathSeg(PathSegN);
        }
    }

    // get mime-type
	TStr FExt = FNm.GetFExt();
	if (FExt == ".htm") { ContTypeStr = THttp::TextHtmlFldVal; }
	else if (FExt == ".html") { ContTypeStr = THttp::TextHtmlFldVal; }
	else if (FExt == ".js") { ContTypeStr = THttp::TextJavaScriptFldVal; }
	else if (FExt == ".css") { ContTypeStr = THttp::TextCssFldVal; }
	else if (FExt == ".ico") { ContTypeStr = THttp::ImageIcoFldVal; }
	else if (FExt == ".png") { ContTypeStr = THttp::ImagePngFldVal; }
	else if (FExt == ".jpg") { ContTypeStr = THttp::ImageJpgFldVal; }
	else if (FExt == ".jpeg") { ContTypeStr = THttp::ImageJpgFldVal; }
	else if (FExt == ".gif") { ContTypeStr = THttp::ImageGifFldVal; }
	else {
		printf("Unknown MIME type for extension '%s' for file '%s'", FExt.CStr(), FNm.CStr());
		ContTypeStr = THttp::AppOctetFldVal; 
	}

    // return stream to the file
    return TFIn::New(FNm);
}

//////////////////////////////////////
// URL-Redirect-Function
TSASFunRedirect::TSASFunRedirect(const TStr& FunNm,
		const TStr& SettingFNm): TSAppSrvFun(FunNm, saotUndef) { 

	printf("Loading redirects %s\n", FunNm.CStr());
	TFIn FIn(SettingFNm); TStr LnStr, OrgFunNm;
	while (FIn.GetNextLn(LnStr)) {
		TStrV PartV;  LnStr.SplitOnAllCh('\t', PartV, false);
		if (PartV.Empty()) { continue; }
		if (PartV[0].Empty()) {
			// parameters
			EAssert(PartV.Len() >= 3);
			TStr FldNm = PartV[1];
			TStr FldVal = PartV[2];
			if (FldVal.IsPrefix("$")) {
				MapH.GetDat(OrgFunNm).FldNmMapH.AddDat(FldVal.Right(1), FldNm);
			} else {
				MapH.GetDat(OrgFunNm).FldNmValPrV.Add(TStrKd(FldNm, FldVal));
			}
		} else {
			// new function
			EAssert(PartV.Len() >= 2);
			OrgFunNm = PartV[0];
			MapH.AddDat(OrgFunNm).FunNm = PartV[1];
			printf("  %s - %s\n", PartV[0].CStr(), PartV[1].CStr());
		}
	}
	printf("Done\n");
}

void TSASFunRedirect::Exec(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv) {
    PUrl Url = RqEnv->GetHttpRq()->GetUrl();
    TStr FunNm = (Url->GetPathSegs() > 1) ? Url->GetPathSeg(1) : "";
	EAssert(MapH.IsKey(FunNm));
	const TRedirect& Redirect =  MapH.GetDat(FunNm);
	TStrKdV _FldNmValPrV = Redirect.FldNmValPrV;
	for (int FldN = 0; FldN < FldNmValPrV.Len(); FldN++) {
		TStr OrgFldNm = FldNmValPrV[FldN].Key;
		if (Redirect.FldNmMapH.IsKey(OrgFldNm)) {
			_FldNmValPrV.Add(TStrKd(Redirect.FldNmMapH.GetDat(OrgFldNm), FldNmValPrV[FldN].Dat));
		}
	}
	RqEnv->ExecFun(Redirect.FunNm, _FldNmValPrV);
}
