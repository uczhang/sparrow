#include <iostream>
#include "queue.h"
#include <thread>
#include "application .h"
#include "defines.h"
#include "nanolog.hpp"
#include <memory>
#include "json/nlohmann_json.hpp"

nlohmann::json cfg;
inline static void from_file(const std::string& file_path, nlohmann::json& j) {
    std::ifstream i(file_path.c_str());
    i >> j;
    i.close();
}

int main() {

    from_file("./cfg/chameleon.cfg", cfg);

    nanolog::Logger::initialize(nanolog::GuaranteedLogger(), cfg.at("log_path"), cfg.at("log_name"), cfg.at("log_roll_size"));

    Application &inst = Application::instance();
    inst.set_context([](const RequestDeliver& req)->bool{

        auto rsp = std::make_unique<ResponseDeliver>(req.get_owner());

        rsp->get_body().status = 200;
        rsp->get_body().msg_id = req.get_body().msg_id;
        rsp->get_body().msg_type = req.get_body().content_type;
        rsp->get_body().svc_type = req.get_body().svc_type;
        rsp->get_body().token_id = req.get_body().token_id; 
        rsp->get_body().session_id = req.get_body().session_id;
        rsp->get_body().kernel_id = "k02";
        rsp->get_body().detail = "it's ok"; 
        rsp->get_body().inspect_id = "183983488384";
        rsp->get_body().esStatisticsParam = "10";
        rsp->get_body().text_is_sync = req.get_body().text_is_sync;
        send_rsp_msg(std::move(rsp));



        auto audit = std::make_unique<RequestAudit>();
        audit->get_body().msg_id = req.get_body().msg_id;
        audit->get_body().msg_type = req.get_body().content_type;
        audit->get_body().hash = req.get_body().hash;
        audit->get_body().policy_type = 1;
        audit->get_body().policy_id = "A123233424234234";
        audit->get_body().report_type = 2;
        audit->get_body().svc_type = req.get_body().svc_type;
        audit->get_body().token_id = req.get_body().token_id;
        audit->get_body().from = req.get_body().from;
        audit->get_body().to = req.get_body().to;
        audit->get_body().to_counts = req.get_body().counts;
        send_adt_msg(audit);

        return true;
    });

    inst.start();

    return 0;
}
