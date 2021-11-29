#include "module.hh"

#include "fmt/format.h"
#include "scheduler.hh"

namespace xsim::runtime {

bool trigger_posedge(const logic::logic<0> &old, const logic::logic<0> &new_) {
    // LRM Table 9-2
    return ((old != logic::logic<0>::one_() && new_ == logic::logic<0>::one_()) ||
            (old == logic::logic<0>::zero_() && new_ != logic::logic<0>::zero_()));
}

bool trigger_negedge(const logic::logic<0> &old, const logic::logic<0> &new_) {
    // LRM Table 9-2
    return ((old != logic::logic<0>::zero_() && new_ == logic::logic<0>::zero_()) ||
            (old == logic::logic<0>::one_() && new_ != logic::logic<0>::one_()));
}

std::string Module::hierarchy_name() const {
    std::string result = std::string(inst_name);
    auto const *module = this->parent;
    while (module) {
        result = fmt::format("{0}.{1}", module->inst_name, result);
        module = module->parent;
    }

    return result;
}

class CombinationalGraph {
public:
    explicit CombinationalGraph(const std::vector<CombProcess *> &processes)
        : processes_(processes) {}

    void run() {
        for (auto *p : processes_) {
            // if it's not finished, it means it's waiting
            if (!p->finished) {
                continue;
            }

            marl::schedule([p]() {
                p->finished = false;
                p->running = true;
                p->func();
                p->cond.signal();
                p->finished = true;
                p->running = false;
            });
            p->cond.wait();
            p->running = false;
        }

        // after every process have been run, we cancel the changes
        // maybe adjust this in the future once we have instances?
        for (auto *p : processes_) {
            p->cancel_changed();
        }
    }

private:
    const std::vector<CombProcess *> &processes_;
};

void Module::active() {
    if (!comb_graph_) {
        comb_graph_ = std::make_shared<CombinationalGraph>(comb_processes_);
        // every process is finished by default
        for (auto *p : comb_processes_) {
            p->finished = true;
        }
    }

    // try to finish what's still there
    wait_for_timed_processes();

    while (!sensitivity_stable()) {
        comb_graph_->run();
    }
}

bool Module::sensitivity_stable() {
    auto r = std::all_of(comb_processes_.begin(), comb_processes_.end(),
                         [](auto *p) { return !p->input_changed(); });
    return r;
}

bool Module::stabilized() const {
    return std::all_of(comb_processes_.begin(), comb_processes_.end(),
                       [](auto *p) { return p->finished || !p->running; });
}

void Module::wait_for_timed_processes() {
    for (auto *p : comb_processes_) {
        if (!p->finished && p->running) {
            p->cond.wait();
            p->running = false;
        }
    }

    for (auto *p : ff_process_) {
        if (!p->finished && p->running) {
            p->cond.wait();
        }
    }
}

}  // namespace xsim::runtime