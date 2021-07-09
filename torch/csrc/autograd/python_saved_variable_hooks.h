#pragma once

#include <pybind11/pybind11.h>
#include <torch/csrc/autograd/python_variable.h>
#include <torch/csrc/autograd/saved_variable_hooks.h>
#include <torch/csrc/python_headers.h>
#include <ATen/ATen.h>

namespace py = pybind11;

namespace torch { namespace autograd {

struct TORCH_API PySavedVariableHooks : public SavedVariableHooks {
    PySavedVariableHooks(py::function &pack_hook, py::function &unpack_hook) : pack_hook_(pack_hook), unpack_hook_(unpack_hook){}

    void call_pack_hook(at::Tensor &tensor) override {
      py::gil_scoped_acquire acquire;
      // Here, the output of pack_hook_ is a py::object, that we steal using `.release()`
      // so we don't need to increment the ref count.
      data_ = pack_hook_(py::reinterpret_steal<py::object>(THPVariable_Wrap(tensor))).release().ptr();
    }

    at::Tensor call_unpack_hook() override {
      py::gil_scoped_acquire acquire;
      auto res = unpack_hook_(py::cast<py::object>(data_));
      return THPVariable_Unpack(res.ptr());
    }

    ~PySavedVariableHooks() override {
      if (data_) {
        Py_DECREF(data_);
      }
    };

  private:
    py::function pack_hook_;
    py::function unpack_hook_;
    PyObject* data_ = nullptr;
};

}}