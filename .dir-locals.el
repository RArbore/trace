;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((nil . (
	 (eval . (let
		     ((root
		       (projectile-project-root)))
		   (setq-local flycheck-clang-args
			       (list
				(concat "-I../imgui")
				(concat "-I../imgui/backends")
				(concat "-I../tracy/public/tracy")
				(concat "-std=c++20")))
		   (setq-local flycheck-clang-include-path
			       (list
				(concat "../imgui")
				(concat "../imgui/backends")
				(concat "../tracy/public/tracy")
				))
		   (setq-local flycheck-gcc-args
			       (list
				(concat "-I../imgui")
				(concat "-I../imgui/backends")
				(concat "-I../tracy/public/tracy")
				(concat "-std=c++20")
				))
		   (setq-local flycheck-gcc-include-path
			       (list
				(concat "../imgui")
				(concat "../imgui/backends")
				(concat "../tracy/public/tracy")
				))
		   ))))
 (c++-mode . ((c-basic-offset . 4)))
 (c-mode . ((mode . c++))))
