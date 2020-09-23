#pragma once

namespace dx12demo::core
{
	class Application;

	class URootObject
	{
	public:

		URootObject() = default;

		virtual ~URootObject() = default;

	protected:

		Application& GetApp() const;

	private:
		friend class Application;

		static Application& GetApplication();

		static void SetApp(Application*);
	};
}