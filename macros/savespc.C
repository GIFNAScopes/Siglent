
void savespc(const std::string &filename)
{
	readData(filename);
	tree->Draw("C1.Area>>h1(8192)");
	TH1*h1=(TH1*)gDirectory->Get("h1");
	saveSpc(h1,(filename+".spc").c_str());
}
