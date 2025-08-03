#include "../includes/minishell.h"

char	*get_env_value(t_env *env, char *var_name)
{
	if (!env || !var_name || !*var_name)
		return (NULL);
	while (env)
	{
		if (ft_strlen(env->name) == ft_strlen(var_name) && ft_strncmp(env->name,
				var_name, ft_strlen(var_name)) == 0)
		{
			return (env->value);
		}
		env = env->next;
	}
	return (NULL);
}

static int	process_quoted_string(char **value, int *len)
{
	char	c;

	c = **value;
	(*value)++;
	while (**value && **value != c)
	{
		(*len)++;
		(*value)++;
	}
	if (**value == c)
		(*value)++;
	return (0);
}

static int	process_env_variable(char **value, int *len)
{
	int		i;
	char	c;
	char	*env_value;

	(*value)++;
	i = 0;
	while (isalnum((*value)[i]) || (*value)[i] == '_')
		i++;
	c = (*value)[i];
	(*value)[i] = '\0';
	env_value = getenv(*value);
	*len += strlen(env_value);
	(*value)[i] = c;
	*value += i;
	return (0);
}

int	mask_len(t_token *token)
{
	int		len;
	char	*value;

	if (!token || !token->value)
		return (0);
	len = 0;
	value = token->value;
	while (*value)
	{
		if (*value == '\'' || *value == '"')
			process_quoted_string(&value, &len);
		else if (*value == '$' && (isalpha(*(value + 1)) || *(value + 1) == '_'))
			process_env_variable(&value, &len);
		else
		{
			len++;
			value++;
		}
	}
	return (len);
}

bool	prev_not_heredoc(t_token *token)
{
	if (!token || !token->prev)
		return (true);
	return (token->prev->type != TOKEN_HEREDOC);
}

bool	prev_not_redirect(t_token *token)
{
	if (!token || !token->prev)
		return (true);
	return (!ft_token_is_redirection(token->prev->type));
}

typedef struct s_expand_ctx
{
	const char	*arg;
	t_env		*env;
	t_token		*token;
	char		*result;
	size_t		i;
	size_t		j;
	int			in_squote;
	int			in_dquote;
}	t_expand_ctx;

static void	init_expand_ctx(t_expand_ctx *ctx, const char *arg, t_env *env, t_token *token)
{
	ctx->arg = arg;
	ctx->env = env;
	ctx->token = token;
	ctx->i = 0;
	ctx->j = 0;
	ctx->in_squote = 0;
	ctx->in_dquote = 0;
}

static int	handle_single_quote(t_expand_ctx *ctx)
{
	if (!ctx->in_dquote)
	{
		ctx->in_squote = !ctx->in_squote;
		ctx->i++;
		return (1);
	}
	return (0);
}

static int	handle_double_quote(t_expand_ctx *ctx)
{
	if (!ctx->in_squote)
	{
		ctx->in_dquote = !ctx->in_dquote;
		ctx->i++;
		return (1);
	}
	return (0);
}

static void	extract_var_name(const char *arg, size_t start, char *var_name)
{
	size_t	var_len;

	var_len = 0;
	while (ft_isalnum(arg[start + var_len]) || arg[start + var_len] == '_')
		var_len++;
	ft_strlcpy(var_name, arg + start, var_len + 1);
}

static int	handle_variable_expansion(t_expand_ctx *ctx)
{
	size_t		var_start;
	char		var_name[128];
	const char	*val;
	size_t		vlen;
	size_t		var_len;

	if (ctx->in_squote || !prev_not_redirect(ctx->token) ||
		(!ft_isalpha(ctx->arg[ctx->i + 1]) && ctx->arg[ctx->i + 1] != '_'))
		return (0);
	var_start = ctx->i + 1;
	var_len = 0;
	while (ft_isalnum(ctx->arg[var_start + var_len]) || 
		   ctx->arg[var_start + var_len] == '_')
		var_len++;
	extract_var_name(ctx->arg, var_start, var_name);
	val = get_env_value(ctx->env, var_name);
	vlen = ft_strlen(val);
	ft_memcpy(ctx->result + ctx->j, val, vlen);
	ctx->j += vlen;
	ctx->i = var_start + var_len;
	return (1);
}

static char	*expand_arg(const char *arg, t_env *env, t_token *token)
{
	t_expand_ctx	ctx;
	size_t			len;

	if (!arg || !*arg || !env)
		return (NULL);
	len = ft_strlen(arg);
	ctx.result = ft_malloc(len * 4 + 1);
	if (!ctx.result)
		return (NULL);
	init_expand_ctx(&ctx, arg, env, token);
	while (ctx.arg[ctx.i])
	{
		if (ctx.arg[ctx.i] == '\'' && handle_single_quote(&ctx))
			continue ;
		if (ctx.arg[ctx.i] == '"' && handle_double_quote(&ctx))
			continue ;
		if (ctx.arg[ctx.i] == '$' && handle_variable_expansion(&ctx))
			continue ;
		ctx.result[ctx.j++] = ctx.arg[ctx.i++];
	}
	ctx.result[ctx.j] = '\0';
	return (ctx.result);
}

void	expand(t_env *env, t_token *token)
{
	t_token	*current;
	char	*expanded;

	if (!env || !token)
		return ;
	current = token;
	while (current && current->type != TOKEN_EOF)
	{
		if (current->type == TOKEN_WORD)
		{
			expanded = expand_arg(current->value, env, current);
			if (expanded)
			{
				free(current->value);
				current->value = expanded;
			}
		}
		current = current->next;
	}
}
