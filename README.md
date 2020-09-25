# xatslsp

Implementing Language Server for Xanadu

## Status

- [x] startup/shutdown
- [x] text changes
- [ ] parsing the source via ATS/Xanadu
- [ ] reporting type checking errors
- [ ] hover hints
- [ ] automatic code completion

## Building

Please ensure you have a recent CMake. For Debian/Ubuntu, you can use
[the Kitware APT repository](https://apt.kitware.com/).

``` shell
mkdir -p .build && cd .build && cmake ..
make
```

## Running tests

In the build directory, run:

``` shell
make test
```

## Setting up

Ensure you have run `sudo make install` in the CMake's build directory
and that you have the `xatsls` binary in your `PATH`.

### Emacs

Please install [`lsp-mode`](https://github.com/emacs-lsp/lsp-mode) first.

``` emacs-lisp
;; NOTE: replace with your own paths
(setenv "ATSHOME" "/.../ats-lang-anairiats-0.2.12")
(setenv "PATSHOME" "/.../ATS-Postiats")
(setenv "XATSHOME" "/.../ATS-Xanadu")

;; setup ats2-mode
(load "/.../ATS-Postiats/utils/emacs/ats2-mode.el")

(require 'lsp-mode)

;; ATS
(defgroup lsp-xats nil
  "LSP support for ATS/Xanadu, using xatsls."
  :group 'lsp-mode
  :link '(url-link "https://github.com/xats-lang/xatslsp"))

;; NOTE: this will be put into lsp-mode's lsp-clients.el someday
(lsp-register-client
 (make-lsp-client :new-connection (lsp-stdio-connection "xatsls")
                  :major-modes '(ats-mode)
                  :language-id 'ats
                  :priority -1
                  :server-id 'xatsls))
(add-to-list 'lsp-language-id-configuration '(ats-mode . "ats-mode"))
(setq lsp-enable-indentation nil)
(add-hook 'ats-mode-hook #'lsp)
```

### Sublime Text

I am not very familiar with ST so here goes:

0. install the ATS syntax highlighting support package
1. install the [LSP](https://github.com/sublimelsp/LSP) package
2. open `LSP Settings` using the command palette
3. add the following `clients` object to the very end (or append a new
   entry to the existing object):

``` json
"clients": {
    "xatsls": {
      "command": ["/usr/local/bin/xatsls"],
      "enabled": true,
      "languageId": "ats",
      "scopes": ["source.ats"],
      "syntaxes": ["Packages/ats-syntax-highlight/ats.tmLanguage"]
    }
}
```

4. note that we need to specify the full path to the `xatsls`
   executable.
5. using the command palette, enable the `xatsls` LSP server globally
