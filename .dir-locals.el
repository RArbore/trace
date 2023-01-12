;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((nil . (
	 (eval . (let
		     ((root
		       (projectile-project-root)))
		   (setq-local flycheck-clang-args
			       (list
				(concat "-Iimgui") (concat "-Iimgui/backends") (concat "-std=c++20")))
		   (setq-local flycheck-clang-include-path
			       (list
				(concat "imgui") (concat "imgui/backends")))
		   (setq-local flycheck-gcc-args
			       (list
				(concat "-Iimgui") (concat "-Iimgui/backends") (concat "-std=c++20")))
		   (setq-local flycheck-gcc-include-path
			       (list
				(concat "imgui") (concat "imgui/backends")))
		   ))))
 (c++-mode . ((c-basic-offset . 4)))
 (c-mode . ((mode . c++))))
