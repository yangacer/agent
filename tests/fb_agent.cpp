#include "agent/agent.hpp"
#include "agent/io_service_pool.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <json_spirit/json_spirit.h>
#include <fstream>

class agent_pool{
private:
	typedef boost::shared_ptr<agent> agent_ptr;
	boost::mutex ag_mutex_;
	std::vector<agent*> agents_list_;
	std::size_t running_agent_;
	const std::size_t agents_num_;
public:
	agent_pool(std::size_t agents_num):
			agents_num_(agents_num), running_agent_(0){
	}
	agent_ptr new_agent(boost::asio::io_service &service){
		boost::mutex::scoped_lock lock(ag_mutex_);
		agent *p_agent = NULL;
		if(running_agent_ >= agents_num_){
			return agent_ptr();
		}
		if(agents_list_.size() > 0){
			p_agent = agents_list_.back();
			agents_list_.pop_back();
		}else{
			p_agent = new agent(service);
		}
		// agent *p_agent = new agent(service);
		agent_ptr sp_agent(p_agent,
				boost::bind(&agent_pool::release_agent, this, _1));
		running_agent_++;
		return sp_agent;
	}
	void release_agent(agent *old_agent){
		boost::mutex::scoped_lock lock(ag_mutex_);
		agents_list_.push_back(old_agent);
		running_agent_--;
		//delete old_agent;
	}
};

class agent_manager{
private:
	typedef boost::shared_ptr<agent> agent_ptr;
	agent_pool agent_pool_;
	boost::mutex isp_mutex_;
	// io_service_pool io_service_pool_;
	// boost::asio::signal_set signals_;
	boost::asio::io_service io_service;
	boost::asio::io_service::work worker;
public:
	// agent_manager(std::size_t io_services_num, std::size_t agents_num):
			// agent_pool_(agents_num),io_service_pool_(io_services_num),
			// signals_(*(io_service_pool_.get_io_service())){
		// signals_.add(SIGINT);
		// signals_.add(SIGTERM);
// #if defined(SIGQUIT)
		// signals_.add(SIGQUIT);
// #endif // defined(SIGQUIT)
		// signals_.async_wait(boost::bind(&agent_manager::stop, this));
	// }
	agent_manager(std::size_t io_services_num, std::size_t agents_num):
			agent_pool_(agents_num),worker(io_service){
	}
	agent_ptr get_agent(){
		return agent_pool_.new_agent(io_service);
	}
	boost::asio::io_service& get_io_service(){
		boost::mutex::scoped_lock lock(isp_mutex_);
		return io_service;
	}
	void run(){
		io_service.run();
		// io_service_pool_.run();
	}
	// void stop(){
		// boost::mutex::scoped_lock lock(isp_mutex_);
		// io_service_pool_.stop();
	// }
};

class fb_agent{
public:
	typedef boost::function<void(boost::system::error_code const &,
		json_spirit::mValue const &)> finish_handler_type;

	fb_agent(agent_manager &agent_mgr):
			agent_mgr_(agent_mgr){
	}

	void register_endpoint(const std::string &url,
		const http::entity::query_map_t &parameter, finish_handler_type handler){
		shared_info_ptr shared_info(new shared_info_type(agent_mgr_.get_io_service()));
		shared_info->url_ = url;
		shared_info->parameter_ = parameter;
		shared_info->handler_ = handler;
		register_endpoint(shared_info);
	}
	void register_endpoint(const char *url,
		const http::entity::query_map_t &parameter, finish_handler_type handler){
		shared_info_ptr shared_info(new shared_info_type(agent_mgr_.get_io_service()));
		shared_info->url_ = url;
		shared_info->parameter_ = parameter;
		shared_info->handler_ = handler;
		register_endpoint(shared_info);
	}
	void register_endpoint(const std::string &url, finish_handler_type handler){
		http::entity::query_map_t parameter;
		shared_info_ptr shared_info(new shared_info_type(agent_mgr_.get_io_service()));
		shared_info->url_ = url;
		shared_info->parameter_ = parameter;
		shared_info->handler_ = handler;
		register_endpoint(shared_info);
	}
	void register_endpoint(const char *url, finish_handler_type handler){
		http::entity::query_map_t parameter;
		shared_info_ptr shared_info(new shared_info_type(agent_mgr_.get_io_service()));
		shared_info->url_ = url;
		shared_info->parameter_ = parameter;
		shared_info->handler_ = handler;
		register_endpoint(shared_info);
	}
private:
	typedef boost::shared_ptr<agent> agent_ptr;
	
	struct shared_info_struct{
		std::string url_;
		http::entity::query_map_t parameter_;
		finish_handler_type handler_;
		agent_ptr p_agent_;
		boost::asio::deadline_timer blocked_timer_;
		
		shared_info_struct(boost::asio::io_service &io_service):
			blocked_timer_(io_service){
		}
	};
	typedef struct shared_info_struct shared_info_type;
	typedef boost::shared_ptr<shared_info_type> shared_info_ptr;
	agent_manager &agent_mgr_;
	
	void register_endpoint(shared_info_ptr shared_info){
		if(shared_info->p_agent_.get() == NULL){
			shared_info->p_agent_ = agent_mgr_.get_agent();
			if(shared_info->p_agent_.get() == NULL){
				std::cerr << "wait: " << shared_info->url_ << std::endl;
				shared_info->blocked_timer_.expires_from_now(boost::posix_time::seconds(5));
				shared_info->blocked_timer_.async_wait(boost::bind(
					&fb_agent::register_endpoint, this, shared_info));
				return;
			}
		}
		http::entity::url tmp_url(shared_info->url_);
		tmp_url.query.query_map.insert(shared_info->parameter_.begin(), shared_info->parameter_.end());
		shared_info->p_agent_->async_get(tmp_url, false, 
			boost::bind(&fb_agent::finish, this, _1,_2,_3,_4, shared_info));
	}

	void finish(boost::system::error_code const &error_code, http::request const &request,
		http::response const &response,  boost::asio::const_buffers_1 buffer,
		shared_info_ptr shared_info){
		json_spirit::mValue json_value;
		if(!error_code) {
			return;
		} else if(error_code != boost::asio::error::eof){
			shared_info->handler_(error_code, json_value);
			return;
		}
		std::string data;
		combine_buffers(buffer, data);
		
		if(data.size() < 1 || !(json_spirit::read(data, json_value)) ||
			json_value.type() == json_spirit::null_type){
			shared_info->handler_(error_code, json_spirit::mValue(json_spirit::null_type));
			return;
		}
		if(json_value.type() != json_spirit::obj_type){
			shared_info->handler_(error_code, json_value);
			return;
		}
		json_spirit::mObject json_obj = json_value.get_obj();
		json_spirit::mObject::iterator it;
		if((it = json_obj.find("error")) == json_obj.end()){
			shared_info->handler_(error_code, json_value);
			return;
		}
		if(it->second.type() != json_spirit::obj_type){
			shared_info->handler_(error_code, json_value);
			return;
		}
		json_obj = it->second.get_obj();
		json_spirit::mObject::iterator tmp_it;
		if((tmp_it = json_obj.find("code")) == json_obj.end() ||
			tmp_it->second.type() != json_spirit::int_type){
			shared_info->handler_(error_code, json_value);
			return;
		}
		int fb_err_code = tmp_it->second.get_int();
		if(fb_err_code == 1 || fb_err_code == 2 || fb_err_code == 4 || fb_err_code == 9 || fb_err_code == 17 || fb_err_code == 613){
			shared_info->blocked_timer_.expires_from_now(boost::posix_time::seconds(120));
				shared_info->blocked_timer_.async_wait(boost::bind(
					&fb_agent::register_endpoint, this, shared_info));
			return;
		}
		shared_info->handler_(error_code, json_value);
	}

	void combine_buffers(boost::asio::const_buffers_1 buffers, std::string &fnl_data){
		using namespace boost::asio;
		const_buffers_1::const_iterator i = buffers.begin();
		char const* data = 0;
		while (i != buffers.end()){
			data = buffer_cast<char const*>(*i);
			if(data != NULL && buffer_size(*i) > 0){
				fnl_data.append(data, buffer_size(*i));
			}
			++i;
		}
	}
};

class fb_crawler_service{
public:
	static const std::string FEED_ENDPOINT;
	static const std::string INBOX_ENDPOINT;
	
	struct user_register_information{
		std::string id_;
		std::string access_token_;
		std::map<std::string, bool> endpoint_;
		
		user_register_information():
			id_(""), access_token_(""){
		}
	};
	typedef struct user_register_information user_reg_info_type;
	typedef std::map<std::string, user_reg_info_type> user_info_type;
	
	fb_crawler_service(agent_manager &agent_mgr, user_reg_info_type &user_info):
			agent_mgr_(agent_mgr),fagent_(agent_mgr_),
		reload_file_timer_(agent_mgr_.get_io_service()){
		init_endpoint_state(user_info.endpoint_);
		users_info_.insert(user_info_type::value_type(user_info.id_, user_info));
		regist_endpoint();
	}
	
	fb_crawler_service(agent_manager &agent_mgr,
		std::vector<user_reg_info_type> &users_info):
			agent_mgr_(agent_mgr),fagent_(agent_mgr_),
		reload_file_timer_(agent_mgr_.get_io_service()){
		for(std::vector<user_reg_info_type>::iterator it = users_info.begin();
			it != users_info.end(); it++){
			init_endpoint_state(it->endpoint_);
			users_info_.insert(user_info_type::value_type(
				it->id_, *it));
		}
		regist_endpoint();
	}
	
	fb_crawler_service(agent_manager &agent_mgr,
		std::string const &user_info_file_path,
		boost::posix_time::seconds reload_span):
		agent_mgr_(agent_mgr),fagent_(agent_mgr_),
		reload_file_timer_(agent_mgr_.get_io_service()){
		load_user_info_file(user_info_file_path, reload_span);
	}
	
	fb_crawler_service(agent_manager &agent_mgr,
		char const *user_info_file_path,
		boost::posix_time::seconds reload_span):
		agent_mgr_(agent_mgr),fagent_(agent_mgr_),
		reload_file_timer_(agent_mgr_.get_io_service()){
		load_user_info_file(user_info_file_path, reload_span);
	}
private:
	static const std::string graph_api_endpoint;
	agent_manager &agent_mgr_;
	fb_agent fagent_;
	boost::mutex um_mutex_;
	user_info_type users_info_;
	boost::asio::deadline_timer reload_file_timer_;
	
	struct shared_info_struct{
		std::string id_;
		std::string access_token_;
		boost::asio::deadline_timer watch_timer_;
		
		shared_info_struct(agent_manager &agent_mgr):
				watch_timer_(agent_mgr.get_io_service()){};
		shared_info_struct(agent_manager &agent_mgr,
			const std::string &id, const std::string &access_token):
				id_(id), access_token_(access_token),
				watch_timer_(agent_mgr.get_io_service()){};
	};
	
	typedef struct shared_info_struct shared_info_type;
	typedef boost::shared_ptr<shared_info_type> shared_info_ptr;
	
	void init_endpoint_state(std::map<std::string, bool> &endpoint){
		for(std::map<std::string, bool>::iterator it = endpoint.begin();
			it != endpoint.end(); it++){
			it->second = false;
		}
	}
	
	void load_user_info_file(std::string user_info_file_path,
		boost::posix_time::seconds reload_span){
		std::ifstream fin(user_info_file_path.c_str(), std::ios::in | std::ios::binary);
		json_spirit::mValue json_value;
		json_spirit::mObject tmp_obj;
		if(!fin.is_open() || !json_spirit::read(fin, json_value) ||
			json_value.type() != json_spirit::obj_type){
			reload_file_timer_.expires_from_now(boost::posix_time::seconds(reload_span));
			reload_file_timer_.async_wait(boost::bind(&fb_crawler_service::load_user_info_file,
				this, user_info_file_path, reload_span));
			return;
		}
		tmp_obj = json_value.get_obj();
		json_spirit::mObject::iterator tmp_it;
		if((tmp_it = tmp_obj.find("user_info_list")) != tmp_obj.end() ||
			tmp_it->second.type() != json_spirit::array_type){
			reload_file_timer_.expires_from_now(boost::posix_time::seconds(reload_span));
			reload_file_timer_.async_wait(boost::bind(&fb_crawler_service::load_user_info_file,
				this, user_info_file_path, reload_span));
			return;
		}
		json_spirit::mArray user_list_arr = tmp_it->second.get_array();
		json_spirit::mObject user_info_obj; 
		user_reg_info_type user_info;
		user_info_type tmp_users_info;
		for(std::size_t i = 0; i < user_list_arr.size(); i++){
			user_info_obj = user_list_arr[i].get_obj();
			if((tmp_it = user_info_obj.find("id")) == user_info_obj.end()){
				continue;
			}
			user_info.id_ = tmp_it->second.get_str();
			if((tmp_it = user_info_obj.find("access_token")) != user_info_obj.end()){
				user_info.access_token_ = tmp_it->second.get_str();
			}
			if((tmp_it = user_info_obj.find("endpoint")) != user_info_obj.end() &&
				tmp_it->second.type() == json_spirit::array_type){
				json_spirit::mArray endpoint_arr = tmp_it->second.get_array();
				for(std::size_t j = 0; j < user_list_arr.size(); j++){
					user_info.endpoint_.insert(
						std::pair<std::string, bool>(endpoint_arr[j].get_str(), false));
				}
			}
			tmp_users_info.insert(user_info_type::value_type(user_info.id_, user_info));
			user_info.id_ = "";
			user_info.access_token_ = "";
			user_info.endpoint_.clear();
		}
		{
			boost::mutex::scoped_lock lock(um_mutex_);
			user_info_type::iterator tmp_it2;
			std::map<std::string, bool>::iterator tmp_it3;
			for(user_info_type::iterator it = tmp_users_info.begin(); it != tmp_users_info.end();
				it++){
				if((tmp_it2 = users_info_.find(it->first)) == users_info_.end()){
					users_info_.insert(user_info_type::value_type(it->first, it->second));
					continue;
				}
				for(std::map<std::string, bool>::iterator it2 = tmp_it2->second.endpoint_.begin();
					it2 != tmp_it2->second.endpoint_.end(); it2++){
					if((tmp_it3 = it->second.endpoint_.find(it2->first)) != it->second.endpoint_.end() &&
						it2->second == true){
						tmp_it3->second = true;
					}
				}
				tmp_it2->second.endpoint_.swap(it->second.endpoint_);
			}
			regist_endpoint();
		}
		reload_file_timer_.expires_from_now(boost::posix_time::seconds(reload_span));
		reload_file_timer_.async_wait(boost::bind(&fb_crawler_service::load_user_info_file,
			this, user_info_file_path, reload_span));
	}
	
	void regist_endpoint(){
		std::map<std::string, bool>::iterator tmp_it;
		for(user_info_type::iterator it = users_info_.begin();
			it != users_info_.end(); it++){
			if((tmp_it = it->second.endpoint_.find(FEED_ENDPOINT)) != it->second.endpoint_.end() &&
				tmp_it->second == false){
				tmp_it->second = true;
				shared_info_ptr shared_info(new shared_info_type(
					agent_mgr_, it->first, it->second.access_token_));
				regist_feed_endpoint(shared_info);
			}
			if((tmp_it = it->second.endpoint_.find(INBOX_ENDPOINT)) != it->second.endpoint_.end() &&
				tmp_it->second == false){
				tmp_it->second = true;
				shared_info_ptr shared_info(new shared_info_type(
					agent_mgr_, it->first, it->second.access_token_));
				regist_inbox_endpoint(shared_info);
			}
		}
	}
	
	void regist_feed_endpoint(shared_info_ptr shared_info){
		std::string url = graph_api_endpoint + "/" + shared_info->id_ + "/" + FEED_ENDPOINT;
		http::entity::query_map_t parameter;
		parameter.insert(http::entity::query_map_t::value_type("access_token", shared_info->access_token_));
		fagent_.register_endpoint(url, parameter, boost::bind(&fb_crawler_service::feed_endpoint_handle,
		this, _1, _2, shared_info));
	}
	
	void regist_inbox_endpoint(shared_info_ptr shared_info){
		std::string url = graph_api_endpoint + "/" + shared_info->id_ + "/" + INBOX_ENDPOINT;
		http::entity::query_map_t parameter;
		parameter.insert(http::entity::query_map_t::value_type("access_token", shared_info->access_token_));
		fagent_.register_endpoint(url, parameter, boost::bind(&fb_crawler_service::inbox_endpoint_handle,
		this, _1, _2, shared_info));
	}
	
	void feed_endpoint_handle(boost::system::error_code const &error_code,
		json_spirit::mValue const &result, shared_info_ptr shared_info){
		boost::mutex::scoped_lock lock(um_mutex_);
		std::cerr << json_spirit::write(result) << std::endl;
		shared_info->watch_timer_.expires_from_now(boost::posix_time::seconds(1));
		shared_info->watch_timer_.async_wait(boost::bind(&fb_crawler_service::regist_feed_endpoint,
		this, shared_info));
	}
	
	void inbox_endpoint_handle(boost::system::error_code const &error_code,
		json_spirit::mValue const &result, shared_info_ptr shared_info){
		boost::mutex::scoped_lock lock(um_mutex_);
		std::cerr << json_spirit::write(result) << std::endl;
		shared_info->watch_timer_.expires_from_now(boost::posix_time::seconds(1));
		shared_info->watch_timer_.async_wait(boost::bind(&fb_crawler_service::regist_inbox_endpoint,
		this, shared_info));
	}
};

const std::string fb_crawler_service::graph_api_endpoint("https://graph.facebook.com");
const std::string fb_crawler_service::FEED_ENDPOINT("feed");
const std::string fb_crawler_service::INBOX_ENDPOINT("inbox");

int main(int argc, char **argv){
	agent_manager agent_mgr(1, 2);
	fb_crawler_service::user_reg_info_type user_info;
	user_info.id_ = "522883650";
	user_info.access_token_ = "AAAEinuSOZAa8BAHcNspVr7YjXyBFCowpCVyysqrXDkmBpKfMDiKy5VnyZAksofN8QKwVSzUIPXi3x2JRvgZBTlwDFXNbIIZD";
	user_info.endpoint_.insert(std::pair<std::string, bool>("feed", false));
	user_info.endpoint_.insert(std::pair<std::string, bool>("inbox", false));
	fb_crawler_service fcs(agent_mgr, user_info);
	agent_mgr.run();
	return 0;
}
