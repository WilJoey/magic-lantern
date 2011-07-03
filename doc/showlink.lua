function ShowLink(addr, name)
	newname = name
	if string.find(addr, "Video") then
		return -- don't show videos
	elseif string.find(addr, "magiclantern.wikia.com") and not string.find(name, "here") then
		newname = "Wiki:" .. name
	else
		-- escape TeX special characters and strip HTTP
		local a = addr
		a = string.gsub(a, [[http://]], [[]])
		a = string.gsub(a, [[https://]], [[]])
		a = string.gsub(a, [[&]], [[\&]])
		a = string.gsub(a, [[_]], [[\_]])
		a = string.gsub(a, [[#]], [[\#]])
		a = string.gsub(a, [[/]], [[/\phantom{-}\hskip-1ex]])
		newname = newname .. [[{ \scriptsize \textlangle ]] .. a .. [[\textrangle }]]
	end
	tex.print([[\oldhref{]] .. addr .. [[}{]] .. newname .. [[}]])
end
