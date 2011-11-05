#pragma once

namespace Launcher
{
	class DisplayBase : public wxObjectRefData
	{
	public:
		DisplayBase()
		{
		}

		virtual ~DisplayBase()
		{
		}

		virtual bool Execute() = 0;
	};
}
