#pragma once

class CDownloadInfo {
public:
	DWORD m_dwTime;
	long long m_llTotalDownloadedLength;
	string m_strSpeed;
	double m_dPercent;
	long long m_llTotalDownloadTime;
	string m_strAverageSpeed;

	CDownloadInfo(){
		m_dwTime = 0;
		m_llTotalDownloadedLength = 0;
		m_strSpeed = "0.0B/S";
		m_dPercent = 0.00;
		m_llTotalDownloadTime= 0;
		m_strAverageSpeed = "0.0B/S";
	}

	CDownloadInfo & operator=(const CDownloadInfo & objDownloadInfo){
		m_dwTime = objDownloadInfo.m_dwTime;
		m_llTotalDownloadedLength = objDownloadInfo.m_llTotalDownloadedLength;
		m_dPercent = objDownloadInfo.m_dPercent;
		m_strSpeed = objDownloadInfo.m_strSpeed.c_str();
		m_llTotalDownloadTime = objDownloadInfo.m_llTotalDownloadTime;
		m_strAverageSpeed = objDownloadInfo.m_strAverageSpeed.c_str();
		return *this;
	}

	long long getAverageSpeed(){
		if (m_llTotalDownloadTime == 0)
		{
			return 0;
		}
		return m_llTotalDownloadedLength/m_llTotalDownloadTime*1000;
	}
};