-- names.lua
-- defines random player names

-- thanks to Chris Pound's language machines


local names = {

	"Aaroy","Aarry","Aart","Aary","Adan","Adrewar","Adrian","Adricar","Adro",
	"Alan","Ales","Alex","Aley","All","Allip","Andriam","Andy","Anter",
	"Antevid","Art","Arth","Barc","Bardond","Bark","Ben","Benjam","Benjan",
	"Bill","Brad","Bradley","Bran","Brandro","Brane","Brek","Bren","Bretewa",
	"Briames","Briane","Bruce","Bruck","Bryan","Carc","Carry","Cary","Chael",
	"Chan","Chardon","Chartha","Chaul","Chricto","Chris","Chur","Clay","Clayne",
	"Cor","Cord","Cordon","Craig","Cran","Dan","Dane","Danis","Dark","Daven",
	"David","Davin","Den","Dendy","Denjam","Der","Derrey","Derry","Douge",
	"Douger","Duan","Duarlos","Duart","Edwark","Erian","Eug","Euge","Eugenry",
	"Euger","Evan","Evank","Fraig","Freg","Garo","Garry","Garryan","Gendron",
	"Gennis","Geor","Geord","Gord","Goreg","Gorge","Gred","Hard","Harry","Hen",
	"Hennist","Hunteph","Ivane","Jack","Jam","James","Jamie","Jamin","Jasony",
	"Jay","Jayne","Jeff","Jer","Jeremy","Jim","Joe","Joel","John","Jos","Juss",
	"Jussell","Justoph","Keither","Ken","Keven","Kevene","Lardon","Logan",
	"Loge","Mark","Marlex","Marrey","Marry","Mary","Mathad","Mathon","Mathony",
	"Matt","Matthew","Mike","Nat","Nathen","Natric","Natthan","Patt","Paul",
	"Per","Pered","Peter","Peterry","Petewar","Phill","Phillex","Philley",
	"Phillia","Phillip","Ran","Rand","Randan","Ray","Ric","Ricark","Ricarle",
	"Ricart","Richael","Richan","Rick","Rob","Robby","Roby","Rogerry","Ros",
	"Rosel","Rossel","Ruben","Russ","Ryan","Ryane","Ryank","Sam","Sames",
	"Samiel","Scot","Scott","Shan","Stephew","Ster","Sterenj","Sterry",
	"Stertha","Steve","Stew","Stuan","Stuard","Stuart","Ted","Thon","Tim","Tob",
	"Toby","Tod","Tom","Tro","Vicarre","Vichael","Vichan","Wad","Wadley",
	"Walteve","Waltewa","Way","Waymon","Will","Willip",

}


function random_name()
  return names[math.random(1, #names)]
end


-- end of file
