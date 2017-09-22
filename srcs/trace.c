/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trace.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jguyet <jguyet@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/06/06 01:48:05 by jguyet            #+#    #+#             */
/*   Updated: 2017/06/06 01:48:06 by jguyet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_trace.h"

t_trace		*singleton_trace(void)
{
	static t_trace	*trace = NULL;

	if (trace != NULL)
		return (trace);
	if (!(trace = (t_trace*)malloc(sizeof(t_trace))))
		return (NULL);
	trace->sequence = 0;
	trace->shost = NULL;
	//trace->destip = NULL;
	trace->port = 80;
	trace->launch = process_traceroute;
	trace->received = 0;
	trace->send = 0;
	trace->timeout.tv_sec = 5;
	trace->pid = (getpid() & 0xFFFF) | 0x8000;
	trace->ttl = 1;
	trace->sweepincrsize = 0;
	trace->mintime = 0;
	trace->totaltime = 0;
	trace->maxtime = 0;
	trace->max_hop = 30;
	trace->protocol = get_protocol(ICMP);
	trace->socket_type = INTERNAL_SOCK_FLUX;
	trace->retry = true;
	trace->use_ip_header = false;
# ifdef __linux__
	trace->use_ip_header = true;
#endif
	trace->dest_ip = NULL;
	trace->sweepminsize = (60 - trace->protocol->len);
	if (trace->use_ip_header)
		trace->sweepminsize -= IPHDR_SIZE;
	trace->sweepmaxsize = trace->sweepminsize;
	struct in_addr local;

	local.s_addr = INADDR_ANY;

	trace->source_ip = ft_strdup(get_hostname_ipv4(&local));
	trace->ping_interval = DEFAULT_PING_INTERVAL;
	if (!load_ip_tab(trace))
		return (NULL);
	return (trace);
}

void		destruct_trace(t_trace *trace)
{
	int i;

	i = 0;
	while (i < FLAGS_SIZE)
	{
		ft_strdel(&trace->flags[i]->name);
		ft_strdel(&trace->flags[i]->error);
		ft_strdel(&trace->flags[i]->help);
		i++;
	}
	free(trace->flags);
	if (trace->shost != NULL)
		ft_strdel(&trace->shost);
	//ft_strdel(&trace->destip);
	free(trace->ip_tab);
	free(trace);
}

BOOLEAN		send_message(t_trace *trace, t_message *message)
{
	int		res;

	trace->send++;
	trace->start_time = get_current_time_millis();
	res = sendto(trace->sock, message->data, message->len, MSG_DONTWAIT, (struct sockaddr*)&trace->addr, sizeof(trace->addr));

		
	ft_printf("whereto: %s\n", inet_ntoa(trace->addr.sin_addr));
	if (res < 0)
	{
		ft_fprintf(1, "ft_traceroute: sendto: Network is unreachable\n");
		return (false);
	}
	if (res != message->len)
	{
		ft_printf("ft_traceroute: wrote %s %d chars, ret=%d\n", trace->shost, message->len, res);
		return (false);
	}
	return (true);
}

BOOLEAN		process_loop(t_trace *trace)
{
	int i;
	//char *save_addr;

	i = 0;
	//save_addr = ft_strdup(trace->destip);
	while (i < NB_TEST_CONNECTION)
	{
		//Open socket connection
		initialize_socket_receiver_connection(trace);
		initialize_socket_sender_connection(trace);

		trace->message = new_message(trace->sweepminsize);
		trace->message->serialize(trace->message, trace);

		send_message(trace, trace->message);
		if ((trace->ip_tab[i] = handle_message(trace)) != NULL)
		{
			if (ft_strcmp(trace->ip_tab[i], trace->dest_ip) == 0)
				trace->retry = false;
		}
		else
		{
			ft_printf(" *");
		}
		free(trace->message->data);
		free(trace->message);
		//free(trace->destip);
		//trace->destip = ft_strdup(save_addr);
		trace->sequence++;
		//close socket connection
		close(trace->sock);
		close(trace->sock_snd);
		i++;
	}
	//ft_strdel(&save_addr);
	ft_printf("\n");
	return (trace->retry);
}

BOOLEAN		process_traceroute(t_trace *trace)
{
	ft_printf("host %s\n", trace->source_ip);
	while (trace->ttl <= trace->max_hop && trace->retry)
	{
		reset_ip_tab(trace);
		ft_printf("%2d ", trace->ttl);
		if (process_loop(trace) == false) {
			free_ip_tab(trace);
			break ;
		}
		free_ip_tab(trace);
		trace->ttl++;
	}
	return (true);
}
