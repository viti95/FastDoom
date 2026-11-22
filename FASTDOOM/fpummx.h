void CopyQWordsMMX(void *src, void *dest, int num_qwords);
void CopyOWordsSSE2(void *src, void *dest, int num_owords);
void CopyQWordsFPU(void *src, void *dest, int num_qwords);
void GetCPUID(void);
void GetCPUFeatures(void);
