// MessageBox.h

#pragma once

#include "Service.h"
#include "ui/Window.h"

class GC_MessageBox : public GC_Service
{
	DECLARE_SELF_REGISTRATION(GC_MessageBox);

public:
	GC_MessageBox();
	GC_MessageBox(FromFile);
	virtual ~GC_MessageBox();

	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);

protected:
	class MyPropertySet : public GC_Service::MyPropertySet
	{
		typedef GC_Service::MyPropertySet BASE;
		ObjectProperty _propTitle;
		ObjectProperty _propText;
		ObjectProperty _propOption1;
		ObjectProperty _propOption2;
		ObjectProperty _propOption3;
		ObjectProperty _propOnSelect;
		ObjectProperty _propAutoClose;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

private:
	UI::WindowWeakPtr _msgbox;

	string_t _title;
	string_t _text;
	string_t _option1;
	string_t _option2;
	string_t _option3;
	std::string _scriptOnSelect;
	int _autoClose;

	void OnSelect(int n);
};

// end of file
